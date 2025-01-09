/*
    SPDX-FileCopyrightText: 2015, 2016 Ivan Cukic <ivan.cukic(at)kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "activitiessync_p.h"
#include "plasma-activities-stats-logsettings.h"

#include <QCoreApplication>
#include <QDebug>
#include <QThread>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>

namespace ActivitiesSync
{
typedef std::shared_ptr<KActivities::Consumer> ConsumerPtr;

ConsumerPtr instance()
{
    static std::mutex s_instanceMutex;
    static std::weak_ptr<KActivities::Consumer> s_instance;

    std::unique_lock<std::mutex> locker{s_instanceMutex};

    auto ptr = s_instance.lock();

    if (!ptr) {
        ptr = std::make_shared<KActivities::Consumer>();
        s_instance = ptr;
    }

    return ptr;
}

QString currentActivity(ConsumerPtr &activities)
{
    if (!activities) {
        activities = instance();
    }

    if (activities->serviceStatus() == KActivities::Consumer::Unknown) {
        if (QThread::currentThread()->loopLevel() == 0) {
            // If there's no event loop running, we can't load the cache
            // Query the current activity directly
            QDBusReply<QString> reply = QDBusConnection::sessionBus().call(
                QDBusMessage::createMethodCall(
                    QStringLiteral("org.kde.ActivityManager"),
                    QStringLiteral("/ActivityManager/Activities"),
                    QStringLiteral("org.kde.ActivityManager.Activities"),
                    QStringLiteral("CurrentActivity")
                ));
            if (reply.isValid()) {
                return reply.value();
            } else {
                qCWarning(PLASMA_ACTIVITIES_STATS_LOG) << "Failed to query current activity:" << reply.error().message();
            }
        } else {
            qCDebug(PLASMA_ACTIVITIES_STATS_LOG) << "Queried current activity before service KAMD is loaded";
        }
    }

    return activities->currentActivity();
}

} // namespace ActivitiesSync
