/*
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Lucien Xu <lucien.xu@jollamobile.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef DATABASEMANIPULATIONINTERFACE_P_H
#define DATABASEMANIPULATIONINTERFACE_P_H

#include <QtCore/QtGlobal>
#include <QtSql/QSqlDatabase>

class DatabaseManipulationInterface;
class DatabaseManipulationInterfacePrivate
{
public:
    explicit DatabaseManipulationInterfacePrivate(DatabaseManipulationInterface *q);
    virtual ~DatabaseManipulationInterfacePrivate();
    QSqlDatabase db;
protected:
    DatabaseManipulationInterface * const q_ptr;
private:
    Q_DECLARE_PUBLIC(DatabaseManipulationInterface)
    int dbUserVersion(const QString &serviceName, const QString &dataType) const;
    bool doInsert(const QString &table, const QStringList &keys,
                  const QMap<QString, QVariantList> &entries,
                  bool replace = false);
    bool doUpdate(const QString &table, const QMap<QString, QVariantList> &entries,
                  const QString &primary);
    bool doDelete(const QString &table, const QString &key, const QVariantList &entries);
    bool valid;
};

#endif // DATABASEMANIPULATIONINTERFACE_P_H
