/*
 * Copyright (C) 2015 Jolla Ltd. and/or its subsidiary(-ies).
 * Contributors: Chris Adams <chris.adams@jollamobile.com>
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

#include <QtDebug>
#include <QString>
#include <KDateTime>
#include <incidence.h>

#ifndef KCALID_H
#define KCALID_H

// Every incidence in the m/kcal database is uniquely identified by
// a combination of the series UID and the occurrence's recurrenceId
// which identifies it within that series.

class KCalId
{
public:
    KCalId() {} // default ctor needed for QHash semantics.
    explicit KCalId(const QString &incidenceUid, const KDateTime &incidenceRecurrenceId = KDateTime())
        : uid(incidenceUid), recurrenceId(incidenceRecurrenceId) {}
    explicit KCalId(const KCalCore::Incidence::Ptr &incidence)
        : uid(incidence->uid()), recurrenceId(incidence->recurrenceId()) {}

    QString toString() const
    {
        return QStringLiteral("%1,%2")
               .arg(uid)
               .arg(recurrenceId.isValid() ? recurrenceId.toString() : QString());
    }
    static KCalId fromString(const QString &str)
    {
        int commaIdx = str.lastIndexOf(',');
        if (commaIdx <= 0) {
            qWarning() << Q_FUNC_INFO << "invalid KCalId string:" << str;
            return KCalId();
        }

        if (str.endsWith(',')) {
            // this event doesn't contain a recurrence id.
            return KCalId(str.mid(0, str.size() - 1), KDateTime());
        }

        // this event id consists of uid + recurrence id.
        QString uid = str.mid(0, commaIdx);
        QString rid = str.mid(commaIdx + 1);
        return KCalId(uid, KDateTime::fromString(rid));
    }

    // data
    QString uid;
    KDateTime recurrenceId;
};

inline bool operator==(const KCalId &kcalId1, const KCalId &kcalId2)
{
    return (kcalId1.uid == kcalId2.uid) && (kcalId1.recurrenceId == kcalId2.recurrenceId);
}

inline uint qHash(const KCalId &kcalId)
{
    return qHash(kcalId.uid) ^ qHash(kcalId.recurrenceId.toTime_t());
}

Q_DECLARE_TYPEINFO(KCalId, Q_MOVABLE_TYPE);

#endif // KCALID_H
