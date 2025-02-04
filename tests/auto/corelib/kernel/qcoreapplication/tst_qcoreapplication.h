// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QCOREAPPLICATION_H
#define TST_QCOREAPPLICATION_H

#include <QtCore/QtCore>

class tst_QCoreApplication: public QObject
{
    Q_OBJECT
    void runHelperTest();

private slots:
    void sendEventsOnProcessEvents(); // this must be the first test
    void getSetCheck();
    void qAppName();
    void qAppVersion();
    void argc();
    void postEvent();
    void removePostedEvents();
#if QT_CONFIG(thread)
    void deliverInDefinedOrder();
#endif
    void applicationPid();
#ifdef QT_BUILD_INTERNAL
    void globalPostedEventsCount();
#endif
    void processEventsAlwaysSendsPostedEvents();
#ifdef Q_OS_WIN
    void sendPostedEventsInNativeLoop();
#endif
    void quit();
    void reexec();
    void execAfterExit();
    void eventLoopExecAfterExit();
    void customEventDispatcher();
    void testQuitLock();
    void QTBUG31606_QEventDestructorDeadLock();
    void applicationEventFilters_mainThread();
    void applicationEventFilters_auxThread();
    void threadedEventDelivery_data();
    void threadedEventDelivery();
#if QT_CONFIG(process)
    // also add to tst_qapplication.cpp
    void exitFromEventLoop() { runHelperTest(); }
    void exitFromThread() { runHelperTest(); }
    void exitFromThreadedEventLoop() { runHelperTest(); }
#  if defined(Q_OS_APPLE) && defined(QT_GUI_LIB)
    // QGuiApplication in a thread fails inside Apple libs:
    // *** Assertion failure in -[NSMenu _setMenuName:], NSMenu.m:777
    // *** Terminating app due to uncaught exception 'NSInternalInconsistencyException', reason: 'API misuse: setting the main menu on a non-main thread. Main menu contents should only be modified from the main thread.'
#  else
    void mainAppInAThread() { runHelperTest(); }
#  endif
#endif
    void testTrWithPercantegeAtTheEnd();
#if QT_CONFIG(library)
    void addRemoveLibPaths();
#endif
    void theMainThread();
    void testDeleteLaterFromBeforeOutermostEventLoop();
    void setIndividualAttributes_data();
    void setIndividualAttributes();
    void setMultipleAttributes();
};

#endif // TST_QCOREAPPLICATION_H
