#include "runguard.h"

#include <QtCore/QCryptographicHash>

namespace
{
    QString generateKeyHash(const QString& key, const QString& salt)
    {
        QByteArray data;

        data.append(key.toUtf8());
        data.append(salt.toUtf8());
        data = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();

        return data;
    }
}

RunGuard::RunGuard(const QString& key)
    : key(key),
      memLockKey(generateKeyHash(key, "{DBA85A68-5557-4E2D-A844-33EB301E38CC}")),
      sharedmemKey(generateKeyHash(key, "{95AC45E0-EDBA-4EDE-B118-F32F9224E446}")),
      sharedMem(sharedmemKey),
      memLock(memLockKey, 1)
{
    memLock.acquire();
    {
        QSharedMemory fix(sharedmemKey);    // Fix for *nix: http://habrahabr.ru/post/173281/
        fix.attach();
    }
    memLock.release();
}

RunGuard::~RunGuard()
{
    release();
}

bool RunGuard::isAnotherRunning()
{
    if(sharedMem.isAttached())
        return false;

    memLock.acquire();
    const bool isRunning = sharedMem.attach();
    if(isRunning)
        sharedMem.detach();
    memLock.release();

    return isRunning;
}

bool RunGuard::tryToRun()
{
    if(isAnotherRunning())      // Extra check
        return false;

    memLock.acquire();
    const bool result = sharedMem.create(sizeof(quint64));
    memLock.release();
    if(!result)
    {
        release();
        return false;
    }

    return true;
}

void RunGuard::release()
{
    memLock.acquire();
    if(sharedMem.isAttached())
        sharedMem.detach();
    memLock.release();
}
