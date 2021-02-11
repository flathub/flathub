/*
    Based on kdbusservicetest:
    This file is part of libkdbus

    SPDX-FileCopyrightText: 1999 Waldo Bastian <bastian@kde.org>
    SPDX-FileCopyrightText: 2011 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2011 Kevin Ottens <ervin@kde.org>
    SPDX-FileCopyrightText: 2021 Collabora Ltd.

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMetaObject>
#include <QProcess>
#include <QTimer>

#include <kdbusservice.h>

#include <stdio.h>

class TestObject : public QObject
{
    Q_OBJECT
public:
    TestObject(KDBusService *service)
        : m_service(service)
    {}

    ~TestObject()
    {
    }

private Q_SLOTS:
    void slotActivateRequested(const QStringList &args, const QString &workingDirectory)
    {
        qDebug() << "Application executed with working directory"
          << workingDirectory << ", args" << args;
    }

private:
    KDBusService *m_service;
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCoreApplication::setApplicationName(QStringLiteral("steamlink"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("valvesoftware.com"));

    KDBusService service(KDBusService::Unique);
    TestObject testObject(&service);
    QObject::connect(&service, SIGNAL(activateRequested(QStringList,QString)),
                     &testObject, SLOT(slotActivateRequested(QStringList,QString)));

    qDebug() << "Running.";
    a.exec();
    qDebug() << "Terminating.";

    return 0;
}

#include "kdbusservicetest.moc"
