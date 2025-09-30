#ifndef QTKOHZUMANAGER_H
#define QTKOHZUMANAGER_H

#include <QObject>
#include <memory>
#include <thread>
#include <vector>
#include <boost/asio.hpp>
#include <QList>
#include <QTimer>

class KohzuController;
class ICommunicationClient;
class ProtocolHandler;
class AxisState;

class QtKohzuManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(std::string fullResponse READ getFullResponse WRITE setFullResponse)

public:
    explicit QtKohzuManager(QObject *parent = nullptr);
    ~QtKohzuManager();

    std::string getFullResponse() const { return ""; }
    void setFullResponse(const std::string& resp) { }

public slots:
    void connectToController(const QString& host, quint16 port);
    void disconnectFromController();
    void move(int axisNo, int pulse, int speed, bool isAbsolute);
    void moveOrigin(int axisNo, int speed);
    void setSystem(int axisNo, int systemNo, int value);

    // Slots for MainWindow to manage polling
    void addAxisToPoll(int axisNo);
    void removeAxisToPoll(int axisNo);
    void clearPollAxes();

signals:
    void connectionStatusChanged(bool connected);
    void logMessage(const QString& message);
    void positionUpdated(int axisNo, int positionPulse);

private slots:
    void onControllerResponse(int axisNo, bool isOriginCommand, const std::string& fullResponse, char status);
    void pollPositions();

private:
    void cleanup();

    std::unique_ptr<boost::asio::io_context> ioContext_;
    std::unique_ptr<std::thread> ioThread_;
    std::shared_ptr<ICommunicationClient> client_;
    std::shared_ptr<ProtocolHandler> protocolHandler_;
    std::shared_ptr<AxisState> axisState_;
    std::shared_ptr<KohzuController> kohzuController_;

    QTimer* pollTimer_;
    QList<int> axesToPoll_;
};

#endif // QTKOHZUMANAGER_H

