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

#include <QtGui>
#include "mymodelitem.h"


//---------------------------------------------------------------------------------------
myModelItem::myModelItem(const QFileInfo& fileInfo, myModelItem* parent)
{
    mParent = parent;
    mFileInfo = fileInfo;
    walked = false;
    dirty = false;
    watched = false;

    if(parent)
    {
          parent->addChild(this);
          mAbsFilePath = fileInfo.filePath();
    }
    else
    {
        walked = true;
        mAbsFilePath = "";
    }
}

//---------------------------------------------------------------------------------------
myModelItem::~myModelItem()
{
    qDeleteAll(mChildren);
}

//---------------------------------------------------------------------------------------
myModelItem* myModelItem::childAt(int position)
{
    return mChildren.value(position, nullptr);
}

//---------------------------------------------------------------------------------------
int myModelItem::childCount() const
{
    if(walked) return mChildren.count();
    return 1;
}

//---------------------------------------------------------------------------------------
bool myModelItem::hasChild(QString fileName)
{
    return mChildByName.contains(fileName);
}

//---------------------------------------------------------------------------------------
myModelItem* myModelItem::childByName(const QString &fileName) const
{
    return mChildByName.value(fileName, nullptr);
}

//---------------------------------------------------------------------------------------
int myModelItem::childNumber() const
{
    if(mParent)
    {
      return mParent->mChildren.indexOf(const_cast<myModelItem*>(this));
    }

    return 0;
}

//---------------------------------------------------------------------------------------
QList<myModelItem*> myModelItem::children()
{
    return mChildren;
}

//---------------------------------------------------------------------------------------
myModelItem* myModelItem::parent()
{
    return mParent;
}

//---------------------------------------------------------------------------------------
QString myModelItem::absoluteFilePath()const
{
    return mAbsFilePath;
}

//---------------------------------------------------------------------------------------
QString myModelItem::fileName() const
{
    if(mAbsFilePath == "/") return "/";
    else return mFileInfo.fileName();

}

//---------------------------------------------------------------------------------------
QFileInfo myModelItem::fileInfo() const
{
    return mFileInfo;
}

//---------------------------------------------------------------------------------------
void myModelItem::refreshFileInfo()
{
    mFileInfo.refresh();
    mPermissions.clear();
    mMimeType.clear();
}

//---------------------------------------------------------------------------------------
void myModelItem::addChild(myModelItem *child)
{
    if(!mChildren.contains(child)) {
        mChildren.append(child);
        mChildByName.insert(child->fileName(), child);
    }
}

//---------------------------------------------------------------------------------------
void myModelItem::removeChild(myModelItem *child)
{
    if (!child) {
        return;
    }
    mChildByName.remove(child->fileName());
    mChildren.removeOne(child);
    delete child;
}

//---------------------------------------------------------------------------------------
void myModelItem::clearAll()
{
    foreach(myModelItem *child, mChildren)
        delete child;
    mChildren.clear();
    mChildByName.clear();
    walked = 0;
}

//---------------------------------------------------------------------------------------
void myModelItem::changeName(QString newName)
{
    if (mParent) {
        mParent->mChildByName.remove(fileName());
    }
    mAbsFilePath = mParent->absoluteFilePath() + SEPARATOR + newName;
    mFileInfo.setFile(mAbsFilePath);
    if (mParent) {
        mParent->mChildByName.insert(fileName(), this);
    }
    clearAll();
}

//---------------------------------------------------------------------------------------
myModelItem* myModelItem::matchPath(const QStringList& path, int startIndex)
{
    QStringList temp = path;
    temp.replace(0,"/");
    temp.removeAll("");

    if(walked == 0)     //not populated yet
    {
        walked = true;
        QDir dir(this->absoluteFilePath());
        QFileInfoList all = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot
                                              | QDir::Hidden | QDir::System,
                                              QDir::NoSort);

        foreach(QFileInfo one, all)
            new myModelItem(one,this);
    }

    if (startIndex >= temp.count()) {
        return nullptr;
    }

    myModelItem *child = childByName(temp.at(startIndex));
    if (!child) {
        return nullptr;
    }
    if (startIndex + 1 == temp.count()) {
        return child;
    }
    return child->matchPath(path, startIndex + 1);
}

//---------------------------------------------------------------------------------------
