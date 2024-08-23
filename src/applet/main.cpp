#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QMessageBox>

#include <cstddef>
#include <iostream>

#include "util.hpp"

#define VERSION "1.0.0a"

// executes auto-ryzenadjctl to get profiles and returns them as a vector
QVector<QString> get_profiles() {
    QVector<QString> profiles = {};

    // run command
    std::vector<std::string> lines = exec_ryzenctl({"--listprofiles"});

    // read output line by line 
    for (auto& line : lines) {
        // find position of the seperator ':'
        size_t colon_pos = line.find(":");
        if (colon_pos != std::string::npos) {
            // remove ':' and everything after
            line.erase(colon_pos, line.length());
            profiles.append(QString(line.c_str()));
        }
    }

    return profiles;
}

// handle button press
void on_button_click(const QString &button, QSystemTrayIcon &tray_icon, QMenu &menu) {
    // run process
    std::vector<std::string> lines = exec_ryzenctl({"--setprofile", button.toStdString()});

    // read and parse output 
    std::string output;
    for (auto& line : lines) {
        output += line + "\n";
    }

    // send notification
    tray_icon.showMessage(QString("Set profile to '") + button + QString("'!"), QString(output.c_str()), QSystemTrayIcon::Information, 5000);
}

void refresh_selected(QAction* action) {
    // get status info
    std::vector<std::string> lines = exec_ryzenctl({"--status"});
    // parse line by line
    for (auto& line : lines) {
        // until profile is found
        if (line.find("profile:") == 0) {
            // find position of ':' 
            size_t colon_pos = line.find(":");
            // and erase everything until ':' including itself
            line.erase(0, colon_pos + 1);
            // set text of button
            action->setText("Selected: " + QString(line.c_str()));
            break;
        }
    }
}

// refresh the menu with profiles
void refresh_menu(QSystemTrayIcon &tray_icon, QApplication &app, QMenu &menu) {
    // send notification
    tray_icon.showMessage(QString("Refreshing profiles..."), QString("This may take a second."), QSystemTrayIcon::Information, 5000);

    // clear the old actions
    menu.clear();

    // get vector of profiles
    QVector<QString> profiles = get_profiles();

    // create a button to show which profile is selected
    QAction *selected_action = new QAction("Selected: ", &menu);
    QAction::connect(selected_action, &QAction::triggered, [selected_action]() {
        refresh_selected(selected_action);
    });

    // add profiles to the menu
    for (const QString &name : profiles) {
        QAction *action = new QAction(name, &menu);
        QAction::connect(action, &QAction::triggered, [&tray_icon, &menu, name, selected_action]() {
            // button action
            on_button_click(name, tray_icon, menu);
            // refresh selected after button action
            refresh_selected(selected_action);
        });
        menu.addAction(action);
    }
    // refresh immediatly otherwise it will be blank
    refresh_selected(selected_action);

    // add the button to the menu
    menu.addAction(selected_action);
    // add a manual refresh button just in case
    QAction *refresh_action = new QAction("refresh", &menu);
    QAction::connect(refresh_action, &QAction::triggered, [&tray_icon, &app, &menu]() {
        refresh_menu(tray_icon, app, menu);
    });
    menu.addAction(refresh_action);
    // add a quit button
    QAction *quitAction = new QAction("Quit", &menu);
    QAction::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);
    menu.addAction(quitAction);
}

int main(int argc, char *argv[]) {
    // set fallback theme, if this is not set the icon will be blank
    QIcon::setFallbackThemeName(QString("breeze"));

    QApplication app(argc, argv);

    // if no systray is available, show a warning and exit
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, "Systray", "No system tray available on this system.");
        return 1;
    }

    // setup icon
    QSystemTrayIcon tray_icon;
    tray_icon.setIcon(QIcon::fromTheme("battery-good"));
    QMenu menu;

    // populate menu
    refresh_menu(tray_icon, app, menu);

    tray_icon.setContextMenu(&menu);
    tray_icon.show();

    return app.exec();
}
