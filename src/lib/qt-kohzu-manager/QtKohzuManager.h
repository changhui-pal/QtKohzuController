#ifndef QTKOHZUMANAGER_H
#define QTKOHZUMANAGER_H

#include <QObject>
#include <memory>
#include <thread>
#include <string>
#include <vector>
#include <QTimer>

#include "controller/KohzuController.h"
#include "controller/AxisState.h"
#include "core/TcpClient.h"
#include "protocol/ProtocolHandler.h"
#include <boost/asio/io_context.hpp>

class QtKohzuManager : public QObject
{
    Q_OBJECT
public:
    explicit QtKohzuManager(QObject *parent = nullptr);
    ~QtKohzuManager();

public slots:
    void connectToServer(const QString &ip, const QString &port);
    void disconnectFromServer();
    void startMonitoring(const std::vector<int>& axes);
    void stopMonitoring();
    void move(int axis, int value, bool isAbsolute);

signals:
    void logMessage(const QString &message);
    void connectionStatusChanged(bool connected);
    void positionUpdated(int axis, int position);

private:
    void closeConnection();

    std::shared_ptr<boost::asio::io_context> io_context_;
    std::thread io_thread_;

    std::shared_ptr<ICommunicationClient> client_;
    std::shared_ptr<ProtocolHandler> protocolHandler_;
    std::shared_ptr<AxisState> axisState_;
    std::shared_ptr<KohzuController> controller_;

    QTimer* monitoringTimer_ = nullptr; // 모니터링 타이머 포인터
};

#endif // QTKOHZUMANAGER_H
