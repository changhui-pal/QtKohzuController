#ifndef QTKOHZUMANAGER_H
#define QTKOHZUMANAGER_H

#include <QObject>
#include <memory>
#include <thread>
#include <vector>
#include <boost/asio.hpp>

// Forward declarations
class KohzuController;
class ICommunicationClient;
class ProtocolHandler;
class AxisState;

class QtKohzuManager : public QObject
{
    Q_OBJECT
public:
    explicit QtKohzuManager(QObject *parent = nullptr);
    ~QtKohzuManager();

    void setSystem(int axisNo, int systemNo, int value);

public slots:
    void connectToController(const QString& host, quint16 port);
    void disconnectFromController();
    void move(int axis_no, int pulse, int speed, bool is_absolute);
    void moveOrigin(int axis_no, int speed);
    void startMonitoring(const std::vector<int>& axes);
    void stopMonitoring();

signals:
    void connectionStatusChanged(bool connected);
    void logMessage(const QString& message);
    void positionUpdated(int axis, int position_pulse);

private:
    void onControllerResponse(int axis_no, bool is_origin_command, const std::string& full_response, char status);
    void cleanup();

    std::unique_ptr<boost::asio::io_context> io_context_;
    std::unique_ptr<std::thread> io_thread_;

    std::shared_ptr<ICommunicationClient> client_;
    std::shared_ptr<ProtocolHandler> protocol_handler_;
    std::shared_ptr<AxisState> axis_state_;
    std::shared_ptr<KohzuController> kohzu_controller_;

    QTimer* monitor_timer_;
};

#endif // QTKOHZUMANAGER_H

