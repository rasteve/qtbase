// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvxmousemanager_p.h"

#include <QtInputSupport/private/qevdevutil_p.h>

#include <QStringList>
#include <QGuiApplication>
#include <QScreen>
#include <QLoggingCategory>
#include <qpa/qwindowsysteminterface.h>
#include <QtDeviceDiscoverySupport/private/qdevicediscovery_p.h>
#include <private/qguiapplication_p.h>
#include <private/qinputdevicemanager_p_p.h>
#include <private/qhighdpiscaling_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QVxMouseManager::QVxMouseManager(const QString &key, const QString &specification, QObject *parent)
    : QObject(parent), m_x(0), m_y(0), m_xoffset(0), m_yoffset(0)
{
    Q_UNUSED(key);

    QString spec = QString::fromLocal8Bit(qgetenv("QT_QPA_VXEVDEV_MOUSE_PARAMETERS"));

    if (spec.isEmpty())
        spec = specification;

    auto parsed = QEvdevUtil::parseSpecification(spec);
    m_spec = std::move(parsed.spec);

    for (const auto &arg : std::as_const(parsed.args)) {
        if (arg.startsWith("xoffset="_L1)) {
            m_xoffset = arg.mid(8).toInt();
        } else if (arg.startsWith("yoffset="_L1)) {
            m_yoffset = arg.mid(8).toInt();
        }
    }

    // add all mice for devices specified in the argument list
    for (const QString &device : std::as_const(parsed.devices))
        addMouse(device);

    if (parsed.devices.isEmpty()) {
        qCDebug(qLcVxMouse, "vxmouse: Using device discovery");
        if (auto deviceDiscovery = QDeviceDiscovery::create(QDeviceDiscovery::Device_Mouse | QDeviceDiscovery::Device_Touchpad, this)) {
            // scan and add already connected keyboards
            const QStringList devices = deviceDiscovery->scanConnectedDevices();
            for (const QString &device : devices)
                addMouse(device);

            connect(deviceDiscovery, &QDeviceDiscovery::deviceDetected,
                    this, &QVxMouseManager::addMouse);
            connect(deviceDiscovery, &QDeviceDiscovery::deviceRemoved,
                    this, &QVxMouseManager::removeMouse);
        }
    }

    QInputDeviceManager *manager = QGuiApplicationPrivate::inputDeviceManager();
    connect(manager, &QInputDeviceManager::cursorPositionChangeRequested, this, [this](const QPoint &pos) {
        m_x = pos.x();
        m_y = pos.y();
        clampPosition();
    });
}

QVxMouseManager::~QVxMouseManager()
{
}

void QVxMouseManager::clampPosition()
{
    // clamp to screen geometry
    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    QRect g = QHighDpi::toNativePixels(primaryScreen->virtualGeometry(), primaryScreen);
    if (m_x + m_xoffset < g.left())
        m_x = g.left() - m_xoffset;
    else if (m_x + m_xoffset > g.right())
        m_x = g.right() - m_xoffset;

    if (m_y + m_yoffset < g.top())
        m_y = g.top() - m_yoffset;
    else if (m_y + m_yoffset > g.bottom())
        m_y = g.bottom() - m_yoffset;
}

void QVxMouseManager::handleMouseEvent(int x, int y, bool abs, Qt::MouseButtons buttons,
                                          Qt::MouseButton button, QEvent::Type type)
{
    // update current absolute coordinates
    if (!abs) {
        m_x += x;
        m_y += y;
    } else {
        m_x = x;
        m_y = y;
    }

    clampPosition();

    QPoint pos(m_x + m_xoffset, m_y + m_yoffset);
    // Cannot track the keyboard modifiers ourselves here. Instead, report the
    // modifiers from the last key event that has been seen by QGuiApplication.
    QWindowSystemInterface::handleMouseEvent(0, pos, pos, buttons, button, type, QGuiApplicationPrivate::inputDeviceManager()->keyboardModifiers());
}

void QVxMouseManager::handleWheelEvent(QPoint delta)
{
    QPoint pos(m_x + m_xoffset, m_y + m_yoffset);
    QWindowSystemInterface::handleWheelEvent(0, pos, pos, QPoint(), delta, QGuiApplicationPrivate::inputDeviceManager()->keyboardModifiers());
}

void QVxMouseManager::addMouse(const QString &deviceNode)
{
    qCDebug(qLcVxMouse, "Adding mouse at %ls", qUtf16Printable(deviceNode));
    auto handler = QVxMouseHandler::create(deviceNode, m_spec);
    if (handler) {
        connect(handler.get(), &QVxMouseHandler::handleMouseEvent,
                this, &QVxMouseManager::handleMouseEvent);
        connect(handler.get(), &QVxMouseHandler::handleWheelEvent,
                this, &QVxMouseManager::handleWheelEvent);
        m_mice.add(deviceNode, std::move(handler));
        updateDeviceCount();
    } else {
        qWarning("vxmouse: Failed to open mouse device %ls", qUtf16Printable(deviceNode));
    }
}

void QVxMouseManager::removeMouse(const QString &deviceNode)
{
    if (m_mice.remove(deviceNode)) {
        qCDebug(qLcVxMouse, "Removing mouse at %ls", qUtf16Printable(deviceNode));
        updateDeviceCount();
    }
}

void QVxMouseManager::updateDeviceCount()
{
    QInputDeviceManagerPrivate::get(QGuiApplicationPrivate::inputDeviceManager())->setDeviceCount(
        QInputDeviceManager::DeviceTypePointer, m_mice.count());
}

QT_END_NAMESPACE
