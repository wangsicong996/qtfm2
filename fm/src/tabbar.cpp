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

#include "tabbar.h"
#include "bundledicons.h"
#include <QMenu>

//---------------------------------------------------------------------------
tabBar::tabBar(QHash<QString,QIcon> * icons)
{
    folderIcons = icons;
    setAcceptDrops(1);
    setMovable(1);
    connect(this, &QTabBar::tabMoved, this, &tabBar::onTabMoved);
}

//---------------------------------------------------------------------------
void tabBar::mousePressEvent(QMouseEvent * event)
{
    //middle-click to close tab
    if(event->button() == Qt::MiddleButton)
    {
        int tab = tabAt(event->pos());
        if(tab != -1)
        {
            emit closeTabRequested(tab);
        }
    }
    else
    if(event->button() == Qt::RightButton)
    {
        int tab = tabAt(event->pos());
        if(tab != -1)
        {
            this->setCurrentIndex(tab);
            QMenu menu(this);
            QAction *newWinAction = menu.addAction(
                BundledIcons::toolbarIcon(QStringLiteral("window-new")),
                tr("Open in new window"));
            if (menu.exec(event->globalPos()) == newWinAction) {
                emit openInNewWindowRequested(tab);
            }
        }
    }

    return QTabBar::mousePressEvent(event);
}

//---------------------------------------------------------------------------
void tabBar::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasFormat("text/uri-list"))
        event->acceptProposedAction();
}

//---------------------------------------------------------------------------
void tabBar::dragMoveEvent(QDragMoveEvent *event)
{
    this->setCurrentIndex(tabAt(event->pos()));
    event->acceptProposedAction();
}

//---------------------------------------------------------------------------
void tabBar::dropEvent(QDropEvent *event)
{
    QList<QUrl> paths = event->mimeData()->urls();
    QFileInfo file = QFileInfo(paths.at(0).path());

    if(tabAt(event->pos()) == -1 && file.isDir())           //new tab
        addNewTab(file.filePath(),0);
    else
    {
        QStringList cutList;

        //don't do anything if you drag and drop in same folder
        if(file.canonicalPath() == tabData(currentIndex()).toString())
        {
            event->ignore();
            return;
        }

        if (event->proposedAction() == 2) { //cut, holding ctrl to copy is action 1
            foreach (QUrl item, paths) { cutList.append(item.path()); }
        }

        emit dragDropTab(event->mimeData(), tabData(currentIndex()).toString(), cutList);
    }

    event->acceptProposedAction();
}

//---------------------------------------------------------------------------
int tabBar::addNewTab(QString path, int type)
{
    QFileInfo file(path);

    history.append(new QStringList(path));
    viewType.append(type);

    TabPaneSession session;
    session.leftPath = file.filePath();
    session.leftHistory = QStringList{file.filePath()};
    paneSessions.append(session);

    QString filename = file.fileName();
    if (filename.isEmpty()) { filename = "/"; }

    int newtab = addTab(filename);
    setTabData(newtab,file.filePath());
    setIcon(newtab);
    if (this->count()>1 && this->isHidden()) { this->show(); }
    return newtab;
}

//---------------------------------------------------------------------------
void tabBar::setIcon(int index)
{
    if(folderIcons->contains(tabText(index))) setTabIcon(index,folderIcons->value(tabText(index)));
    else setTabIcon(index, BundledIcons::iconByName(QStringLiteral("folder")));
}

//---------------------------------------------------------------------------
void tabBar::addHistory(QString path)
{
    history.at(currentIndex())->insert(0,path);
}

//---------------------------------------------------------------------------
void tabBar::remHistory()
{
    history.at(currentIndex())->removeFirst();
}

//---------------------------------------------------------------------------
QStringList *tabBar::getHistory(int index)
{
    return history.value(index);
}

//---------------------------------------------------------------------------
int tabBar::getType(int index)
{
    return viewType.at(index);
}

//---------------------------------------------------------------------------
void tabBar::setType(int type)
{
    viewType.removeAt(currentIndex());
    viewType.insert(currentIndex(),type);
}

//---------------------------------------------------------------------------
TabPaneSession *tabBar::sessionAt(int index)
{
    if (index < 0 || index >= paneSessions.size()) {
        return nullptr;
    }
    return &paneSessions[index];
}

//---------------------------------------------------------------------------
void tabBar::initSessionSingle(int index, const QString &path)
{
    TabPaneSession *s = sessionAt(index);
    if (!s) {
        return;
    }
    *s = TabPaneSession();
    s->leftPath = path;
    s->leftHistory = QStringList{path};
    s->dualPane = false;
    s->activePane = 0;
}

//---------------------------------------------------------------------------
void tabBar::removeSessionAt(int index)
{
    if (index < 0 || index >= paneSessions.size()) {
        return;
    }
    paneSessions.removeAt(index);
}

//---------------------------------------------------------------------------
void tabBar::onTabMoved(int from, int to)
{
    if (from < 0 || to < 0 || from >= history.size() || to >= history.size()) {
        return;
    }
    history.move(from, to);
    viewType.move(from, to);
    if (from < paneSessions.size() && to < paneSessions.size()) {
        paneSessions.move(from, to);
    }
}

//---------------------------------------------------------------------------
void tabBar::removeTabAt(int index)
{
    if (index < 0 || index >= count()) {
        return;
    }
    delete history.at(index);
    history.removeAt(index);
    viewType.removeAt(index);
    removeSessionAt(index);
    removeTab(index);
    if (count() == 1) {
        hide();
    }
}

//---------------------------------------------------------------------------
void tabBar::closeTab()
{
    if (currentIndex()>-1) {
        emit closeTabRequested(currentIndex());
    }
}
