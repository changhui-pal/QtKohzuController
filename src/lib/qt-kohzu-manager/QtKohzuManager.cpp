#include "QtKohzuManager.h"
#include "controller/KohzuController.h"
#include "core/TcpClient.h"
#include "protocol/ProtocolHandler.h"
#include "controller/AxisState.h"
#include "spdlog/spdlog.h"
#include <QTimer>

QtKohzuManager::QtKohzuManager(QObject *parent) : QObject(parent)
{
    monitor_timer_ = new QTimer(this);
    connect(monitor_timer_, &QTimer::timeout, this, [this](){
        if(axis_state_){
            // This is just a placeholder if needed for Qt-based periodic checks.
            // Main monitoring is done inside KohzuController's thread.
        }
    });
}

QtKohzuManager::~QtKohzuManager()
{
    cleanup();
}

void QtKohzuManager::cleanup()
{
    stopMonitoring();
    if (io_context_) {
        io_context_->stop();
    }
    // GUI 스레드를 차단하지 않도록 join() 대신 detach()를 사용합니다.
    if (io_thread_ && io_thread_->joinable()) {
        io_thread_->detach();
    }
    io_thread_.reset();
    kohzu_controller_.reset();
    axis_state_.reset();
    protocol_handler_.reset();
    client_.reset();
    io_context_.reset();
}

void QtKohzuManager::move(int axis_no, int pulse, int speed, bool is_absolute)
{
    if (!kohzu_controller_) return;

    auto callback = [this, axis_no](const ProtocolResponse& resp) {
        // 이동 완료 후 200ms 지연을 두고 모니터링을 중지합니다.
        QTimer::singleShot(200, this, &QtKohzuManager::stopMonitoring);
        QMetaObject::invokeMethod(this, "onControllerResponse", Qt::QueuedConnection,
                                  Q_ARG(int, axis_no),
                                  Q_ARG(bool, false),
                                  Q_ARG(std::string, resp.fullResponse),
                                  Q_ARG(char, resp.status));
    };

    if (is_absolute) {
        kohzu_controller_->moveAbsolute(axis_no, pulse, speed, 0, callback);
    } else {
        kohzu_controller_->moveRelative(axis_no, pulse, speed, 0, callback);
    }
}

void QtKohzuManager::moveOrigin(int axis_no, int speed)
{
    if (!kohzu_controller_) return;

    auto callback = [this, axis_no](const ProtocolResponse& resp) {
        // 원점 복귀 완료 후 200ms 지연을 두고 모니터링을 중지합니다.
        QTimer::singleShot(200, this, &QtKohzuManager::stopMonitoring);
        QMetaObject::invokeMethod(this, "onControllerResponse", Qt::QueuedConnection,
                                  Q_ARG(int, axis_no),
                                  Q_ARG(bool, true),
                                  Q_ARG(std::string, resp.fullResponse),
                                  Q_ARG(char, resp.status));
    };

    kohzu_controller_->moveOrigin(axis_no, speed, 0, callback);
}


// ... (connectToController, disconnectFromController, setSystem, startMonitoring, stopMonitoring, onControllerResponse methods are unchanged)
void QtKohzuManager::connectToController(const QString &host, quint16 port)
{
    try {
        cleanup();
        io_context_ = std::make_unique<boost::asio::io_context>();
        client_ = std::make_shared<TcpClient>(*io_context_, host.toStdString(), QString::number(port).toStdString());
        client_->connect(host.toStdString(), QString::number(port).toStdString());

        protocol_handler_ = std::make_shared<ProtocolHandler>(client_);
        axis_state_ = std::make_shared<AxisState>();
        kohzu_controller_ = std::make_shared<KohzuController>(protocol_handler_, axis_state_);

        io_thread_ = std::make_unique<std::thread>([this]() {
            try {
                if(io_context_) io_context_->run();
            } catch (const std::exception& e) {
                spdlog::error("io_context exception: {}", e.what());
            }
        });

        kohzu_controller_->start();
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

void QtKohzuManager::setSystem(int axis_no, int system_no, int value)
{
    if (!kohzu_controller_) return;

    auto callback = [this, axis_no](const ProtocolResponse& resp) {
        QMetaObject::invokeMethod(this, "onControllerResponse", Qt::QueuedConnection,
                                  Q_ARG(int, axis_no),
                                  Q_ARG(bool, false), // Or handle as a specific command type
                                  Q_ARG(std::string, resp.fullResponse),
                                  Q_ARG(char, resp.status));
    };

    kohzu_controller_->setSystem(axis_no, system_no, value, callback);
}


void QtKohzuManager::startMonitoring(const std::vector<int> &axes)
{
    if (!kohzu_controller_) return;

    if(monitor_timer_->isActive()) {
        disconnect(monitor_timer_, &QTimer::timeout, nullptr, nullptr);
        monitor_timer_->stop();
    }

    connect(monitor_timer_, &QTimer::timeout, this, [this, axes](){
        if(axis_state_){
            for(int axis : axes){
                int pos = axis_state_->getPosition(axis);
                emit positionUpdated(axis, pos);
            }
        }
    });
    monitor_timer_->start(100);

    kohzu_controller_->startMonitoring(axes, 100);
}

void QtKohzuManager::stopMonitoring()
{
    if(monitor_timer_->isActive()) {
        disconnect(monitor_timer_, &QTimer::timeout, nullptr, nullptr);
        monitor_timer_->stop();
    }
    if (kohzu_controller_) {
        kohzu_controller_->stopMonitoring();
    }
}

void QtKohzuManager::onControllerResponse(int axis_no, bool is_origin_command, const std::string& full_response, char status)
{
    QString command_type = is_origin_command ? "Origin" : "Move";
    // Check for other command types if necessary
    if (QString::fromStdString(full_response).contains("WSY")) {
        command_type = "Set System";
    }

    QString message = QString("Axis %1 %2 command %3. Response: %4")
                          .arg(axis_no)
                          .arg(command_type)
                          .arg(status == 'C' ? "completed" : "failed")
                          .arg(QString::fromStdString(full_response).trimmed());
    emit logMessage(message);
}
