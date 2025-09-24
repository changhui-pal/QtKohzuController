#include "QtKohzuManager.h"
#include "spdlog/spdlog.h"

QtKohzuManager::QtKohzuManager(QObject *parent) : QObject(parent)
{
}

QtKohzuManager::~QtKohzuManager()
{
    closeConnection();
}

void QtKohzuManager::connectToServer(const QString &ip, const QString &port)
{
    try {
        emit logMessage(QString("Connecting to %1:%2...").arg(ip, port));

        io_context_ = std::make_shared<boost::asio::io_context>();
        client_ = std::make_shared<TcpClient>(*io_context_, ip.toStdString(), port.toStdString());
        protocolHandler_ = std::make_shared<ProtocolHandler>(client_);
        axisState_ = std::make_shared<AxisState>();
        controller_ = std::make_shared<KohzuController>(protocolHandler_, axisState_);

        client_->connect(ip.toStdString(), port.toStdString());
        controller_->start();

        io_thread_ = std::thread([this]() {
            try {
                io_context_->run();
            } catch (const std::exception& e) {
                spdlog::error("io_context exception: {}", e.what());
            }
        });

        emit logMessage("Connection successful.");
        emit connectionStatusChanged(true);

    } catch (const std::exception &e) {
        emit logMessage(QString("Error: %1").arg(e.what()));
        closeConnection();
        emit connectionStatusChanged(false);
    }
}

void QtKohzuManager::disconnectFromServer()
{
    closeConnection();
    emit logMessage("Disconnected.");
    emit connectionStatusChanged(false);
}

void QtKohzuManager::closeConnection()
{
    stopMonitoring(); // 타이머를 확실히 정리
    if (controller_) controller_->stopMonitoring();
    if (io_context_ && !io_context_->stopped()) {
        io_context_->stop();
    }
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
    controller_.reset();
    axisState_.reset();
    protocolHandler_.reset();
    client_.reset();
    io_context_.reset();
}

void QtKohzuManager::startMonitoring(const std::vector<int>& axes)
{
    if (!controller_) return;

    stopMonitoring(); // 기존 타이머가 있다면 정리

    controller_->startMonitoring(axes, 200); // 200ms period
    emit logMessage("Started monitoring axes.");

    monitoringTimer_ = new QTimer(this);
    connect(monitoringTimer_, &QTimer::timeout, this, [this, axes](){
        for(int axis : axes) {
            // axisState_는 스레드 안전하므로 직접 접근 가능
            int pos = axisState_->getPosition(axis);
            if (pos != -1) { // 유효한 위치 값일 때만 업데이트
                emit positionUpdated(axis, pos);
            }
        }
    });
    monitoringTimer_->start(200); // UI 업데이트 주기
}

void QtKohzuManager::stopMonitoring()
{
    if (controller_) {
        controller_->stopMonitoring();
    }
    if (monitoringTimer_) {
        monitoringTimer_->stop();
        monitoringTimer_->deleteLater();
        monitoringTimer_ = nullptr;
    }
}


void QtKohzuManager::move(int axis, int value, bool isAbsolute)
{
    if (!controller_) return;

    int speed = 0; // Speed can be configured in the UI later
    auto callback = [this, axis](const ProtocolResponse& response) {
        QString message = QString("Axis %1 move command finished. Status: %2")
        .arg(axis)
            .arg(response.status);
        emit logMessage(message);
    };

    if (isAbsolute) {
        emit logMessage(QString("Moving axis %1 to absolute position %2...").arg(axis).arg(value));
        controller_->moveAbsolute(axis, value, speed, 0, callback);
    } else {
        emit logMessage(QString("Moving axis %1 by relative distance %2...").arg(axis).arg(value));
        controller_->moveRelative(axis, value, speed, 0, callback);
    }
}
