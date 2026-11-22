#include "ConnectionBridge.hpp"

#include "../../core/settings/LoggingService.hpp"
#include "../../engines/moonlight/MoonlightAdapter.hpp"
#include "../../engines/rustdesk/RustDeskAdapter.hpp"

#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QStandardPaths>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("UnifiedStreamNext");
    app.setApplicationVersion("0.1.0");

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    usn::settings::LoggingService::instance().configure(
        (dataDir + "/unifiedstreamnext.log").toStdString(),
        usn::settings::LogLevel::Info);

    usn::settings::SettingsManager settingsMgr((dataDir + "/settings.conf").toStdString());
    settingsMgr.load();

    usn::device::DeviceManager deviceMgr((dataDir + "/devices.db").toStdString());
    deviceMgr.load();

    usn::network::NetworkManager networkMgr;
    usn::discovery::DiscoveryService discoverySvc(networkMgr);
    usn::session::SessionManager sessionMgr((dataDir + "/history.db").toStdString());
    sessionMgr.loadHistory();

    usn::connection::ConnectionManager connMgr(deviceMgr, discoverySvc, networkMgr, sessionMgr, settingsMgr);

    const QString appDir = QCoreApplication::applicationDirPath();
    connMgr.registerAdapter(std::make_shared<usn::engines::MoonlightAdapter>(
        (appDir + "/engines/moonlight/Moonlight.exe").toStdString()));
    connMgr.registerAdapter(std::make_shared<usn::engines::RustDeskAdapter>(
        (appDir + "/engines/rustdesk/rustdesk.exe").toStdString()));

    usn::ui::ConnectionBridge bridge(connMgr, deviceMgr, sessionMgr, settingsMgr);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("connectionBridge", &bridge);
    engine.loadFromModule("UnifiedStreamNext", "Main");
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }
    return app.exec();
}
