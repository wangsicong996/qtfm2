/****************************************************************************
* This file is part of qtFM, a simple, fast file manager.
* Copyright (C) 2010,2011,2012 Wittfella
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>
*
* Contact e-mail: wittfella@qtfm.org
*
****************************************************************************/

#ifndef TABBAR_H
#define TABBAR_H

#include <QtGui>
#include <QTabBar>
#include <QByteArray>
#include <QStringList>

/** Per-tab dual-pane / path snapshot (tabs are independent). */
struct TabPaneSession {
    bool dualPane = false;
    int activePane = 0;
    QByteArray splitterState;
    QString leftPath;
    QStringList leftHistory;
    QStringList leftForward;
    QString rightPath;
    QStringList rightHistory;
    QStringList rightForward;
    QString searchFilter;
};

class tabBar : public QTabBar
{
    Q_OBJECT

public:
    tabBar(QHash<QString,QIcon> *);
    int addNewTab(QString path,int type);
    void setIcon(int index);
    void mousePressEvent(QMouseEvent * event);
    void addHistory(QString);
    void remHistory();
    QStringList *getHistory(int);
    int getType(int index);
    void setType(int type);

    TabPaneSession *sessionAt(int index);
    void initSessionSingle(int index, const QString &path);
    void removeSessionAt(int index);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);

signals:
        void dragDropTab(const QMimeData * data, QString newPath, QStringList cutList);
        void openInNewWindowRequested(int index);
        void closeTabRequested(int index);

public slots:
        void closeTab();
        void onTabMoved(int from, int to);
        void removeTabAt(int index);

private:
        QHash<QString,QIcon> *folderIcons;
        QList<QStringList*> history;
        QList<int> viewType;
        QList<TabPaneSession> paneSessions;

};

#endif // TABBAR_H
