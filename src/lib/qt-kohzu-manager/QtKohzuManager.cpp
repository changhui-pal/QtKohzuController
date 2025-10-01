#include "QtKohzuManager.h"
#include "controller/KohzuController.h"
#include "core/TcpClient.h"
#include "protocol/ProtocolHandler.h"
#include "controller/AxisState.h"
#include "spdlog/spdlog.h"

QtKohzuManager::QtKohzuManager(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<std::string>("std::string");
    pollTimer_ = new QTimer(this);
    connect(pollTimer_, &QTimer::timeout, this, &QtKohzuManager::pollPositions);
}

QtKohzuManager::~QtKohzuManager()
{
    cleanup();
}

void QtKohzuManager::connectToController(const QString &host, quint16 port)
{
    try {
        cleanup();
        ioContext_ = std::make_unique<boost::asio::io_context>();
        client_ = std::make_shared<TcpClient>(*ioContext_, host.toStdString(), QString::number(port).toStdString());
        client_->connect(host.toStdString(), QString::number(port).toStdString());

        protocolHandler_ = std::make_shared<ProtocolHandler>(client_);
        axisState_ = std::make_shared<AxisState>();
        kohzuController_ = std::make_shared<KohzuController>(protocolHandler_, axisState_);

        ioThread_ = std::make_unique<std::thread>([this]() {
            try {
                if(ioContext_) {
                    // Prevent io_context::run() from returning immediately if there's no work.
                    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard(ioContext_->get_executor());
                    ioContext_->run();
                }
            } catch (const std::exception& e) {
                spdlog::error("io_context exception: {}", e.what());
            }
        });

        kohzuController_->start();
        // startMonitoring now only takes the period
        kohzuController_->startMonitoring(100);
        pollTimer_->start(100);

        emit connectionStatusChanged(true);
        emit logMessage(QString("Successfully connected to %1:%2").arg(host).arg(port));

    } catch (const std::exception& e) {
        emit logMessage(QString("Connection failed: %1").arg(e.what()));
        emit connectionStatusChanged(false);
    }
}

void QtKohzuManager::disconnectFromController()
{
    cleanup();
    emit connectionStatusChanged(false);
    emit logMessage("Disconnected.");
}

void QtKohzuManager::cleanup()
{
    pollTimer_->stop();

    if (kohzuController_) {
        // Stop the monitoring thread first
        kohzuController_->stopMonitoring();
    }
    if (ioContext_) {
        // Signal the io_context to stop, allowing run() to exit
        ioContext_->stop();
    }
    if (ioThread_ && ioThread_->joinable()) {
        // Wait for the thread to finish gracefully
        ioThread_->join();
    }

    // Reset all resources
    ioThread_.reset();
    kohzuController_.reset();
    axisState_.reset();
    protocolHandler_.reset();
    client_.reset();
    ioContext_.reset();

    // Clear the polling list after everything is cleaned up
    clearPollAxes();
}

void QtKohzuManager::move(int axisNo, int pulse, int speed, bool isAbsolute)
{
    if (!kohzuController_) return;

    kohzuController_->addAxisToMonitor(axisNo);

    auto callback = [this, axisNo](const ProtocolResponse& resp) {
        QMetaObject::invokeMethod(this, "onControllerResponse", Qt::QueuedConnection,
                                  Q_ARG(int, axisNo), Q_ARG(bool, false),
                                  Q_ARG(std::string, resp.fullResponse), Q_ARG(char, resp.status));
    };
    if (isAbsolute) {
        kohzuController_->moveAbsolute(axisNo, pulse, speed, 0, callback);
    } else {
        kohzuController_->moveRelative(axisNo, pulse, speed, 0, callback);
    }
}

void QtKohzuManager::moveOrigin(int axisNo, int speed)
{
    if (!kohzuController_) return;

    kohzuController_->addAxisToMonitor(axisNo);

    auto callback = [this, axisNo](const ProtocolResponse& resp) {
        QMetaObject::invokeMethod(this, "onControllerResponse", Qt::QueuedConnection,
                                  Q_ARG(int, axisNo), Q_ARG(bool, true),
                                  Q_ARG(std::string, resp.fullResponse), Q_ARG(char, resp.status));
    };
    kohzuController_->moveOrigin(axisNo, speed, 0, callback);
}

void QtKohzuManager::setSystem(int axisNo, int systemNo, int value)
{
    if (!kohzuController_) return;

    auto callback = [this, axisNo](const ProtocolResponse& resp) {
        QMetaObject::invokeMethod(this, "onControllerResponse", Qt::QueuedConnection,
                                  Q_ARG(int, axisNo), Q_ARG(bool, false),
                                  Q_ARG(std::string, resp.fullResponse), Q_ARG(char, resp.status));
    };

    kohzuController_->setSystem(axisNo, systemNo, value, callback);
}

void QtKohzuManager::addAxisToPoll(int axisNo)
{
    if (!axesToPoll_.contains(axisNo)) {
        axesToPoll_.append(axisNo);
    }
}

void QtKohzuManager::removeAxisToPoll(int axisNo)
{
    axesToPoll_.removeAll(axisNo);
}

void QtKohzuManager::clearPollAxes()
{
    axesToPoll_.clear();
}

void QtKohzuManager::pollPositions()
{
    if (axisState_) {
        for (int axisNo : std::as_const(axesToPoll_)) {
            int pos = axisState_->getPosition(axisNo);
            emit positionUpdated(axisNo, pos);
        }
    }
}

void QtKohzuManager::onControllerResponse(int axisNo, bool isOriginCommand, const std::string& fullResponse, char status)
{
    QTimer::singleShot(1000, this, [this, axisNo](){
        if(kohzuController_){
            kohzuController_->removeAxisToMonitor(axisNo);
        }
    });

    QString commandType = isOriginCommand ? "Origin" : "Move";
    QString message = QString("Axis %1 %2 command %3. Response: %4")
                          .arg(axisNo)
                          .arg(commandType)
                          .arg(status == 'C' ? "completed" : "failed")
                          .arg(QString::fromStdString(fullResponse).trimmed());
    emit logMessage(message);
}

