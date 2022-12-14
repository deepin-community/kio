/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef KAUTOMOUNT_H
#define KAUTOMOUNT_H

#include <QObject>
#include <QString>

#include <QtGlobal>

#include "kiowidgets_export.h"

#include <memory>

#ifdef Q_OS_UNIX

class KJob;
namespace KIO
{
class Job;
}

#if KIOWIDGETS_ENABLE_DEPRECATED_SINCE(5, 88)

class KAutoMountPrivate;
/**
 * @class KAutoMount kautomount.h <KAutoMount>
 *
 * This class implements synchronous mounting of devices,
 * as well as showing a file-manager window after mounting a device, optionally.
 * It is a wrapper around the asynchronous KIO::special() call for mount,
 * used by KDesktopFileActions.
 *
 * @short This class implements synchronous mounting of devices.
 *
 * @deprecated Since 5.88. There's no direct replacement, Solid::StorageAccess::setup may help
 */
class KIOWIDGETS_EXPORT KAutoMount : public QObject
{
    Q_OBJECT
public:
    /**
     * Mounts a device.
     * @param readonly if true, the device is mounted read-only
     * @param format the file system (e.g. vfat, ext2...) [optional, fstab is used otherwise]
     * @param device the path to the device (e.g. /dev/fd0)
     * @param mountpoint the directory where to mount the device [optional, fstab is used otherwise]
     * @param desktopFile the file the user clicked on - to notify KDirWatch of the fact that
     * it should emit fileDirty for it (to have the icon change)
     * @param show_filemanager_window if true, a file-manager window for that mountpoint is shown after
     * the mount, if successful.
     */
    KIOWIDGETS_DEPRECATED_VERSION(5, 88, "For lack of usage.")
    KAutoMount(bool readonly,
               const QByteArray &format,
               const QString &device,
               const QString &mountpoint,
               const QString &desktopFile,
               bool show_filemanager_window = true);

Q_SIGNALS:
    /** Emitted when the directory has been mounted */
    void finished();
    /** Emitted in case the directory could not been mounted */
    void error();

private:
    /** KAutoMount deletes itself. Don't delete it manually. */
    ~KAutoMount() override;
    Q_PRIVATE_SLOT(d, void slotResult(KJob *))
    friend class KAutoMountPrivate;
    std::unique_ptr<KAutoMountPrivate> const d;
};

class KAutoUnmountPrivate;
/**
 * This class implements synchronous unmounting of devices,
 * It is a wrapper around the asynchronous KIO::special() call for unmount,
 * used by KDesktopFileActions.
 *
 * @short This class implements synchronous unmounting of devices,
 *
 * @deprecated Since 5.88. There's no direct replacement, Solid::StorageAccess::teardown may help
 */
class KIOWIDGETS_EXPORT KAutoUnmount : public QObject
{
    Q_OBJECT
public:
    /**
     * Unmounts a device.
     * @param mountpoint the mount point - KAutoUnmount finds the device from that
     * @param desktopFile the file the user clicked on - to notify KDirWatch of the fact that
     * it should emit fileDirty for it (to have the icon change)
     */
    KIOWIDGETS_DEPRECATED_VERSION(5, 88, "For lack of usage.")
    KAutoUnmount(const QString &mountpoint, const QString &desktopFile);

Q_SIGNALS:
    /** Emitted when the directory has been unmounted */
    void finished();
    /** Emitted in case the directory could not been unmounted */
    void error();

private:
    /** KAutoUnmount deletes itself. Don't delete it manually. */
    ~KAutoUnmount() override;
    Q_PRIVATE_SLOT(d, void slotResult(KJob *))
    friend class KAutoUnmountPrivate;
    std::unique_ptr<KAutoUnmountPrivate> const d;
};
#endif

#endif // Q_OS_UNIX

#endif
