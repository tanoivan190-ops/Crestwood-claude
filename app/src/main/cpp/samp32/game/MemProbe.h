#pragma once

// ============================================================
// MemProbe - "cet octet est-il LISIBLE ?" sans declencher de SIGSEGV.
//
// mincore() ne suffit pas : il repond "present" pour une page mappee PROT_NONE,
// or c'est exactement ce que sont les pages de garde de l'allocateur Scudo
// (Android 11+). La decompression RLE croyait donc pouvoir lire et plantait
// (crestwood_samp_log : SIGSEGV dans memcpy <- RLEDecompress_safe <- LoadFullTexture).
//
// process_vm_readv() sur notre propre processus lit via le noyau : une page non
// lisible renvoie EFAULT au lieu d'un signal. Repli : write() dans un pipe.
// ============================================================

#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <mutex>
#include <sys/mman.h>
#include <sys/uio.h>
#include <unistd.h>

namespace MemProbe
{
    inline bool ReadableByPipe(const void* addr, bool& usable)
    {
        static int fds[2] = { -1, -1 };
        static std::once_flag once;
        static std::mutex mtx;
        std::call_once(once, [] { if (pipe2(fds, O_NONBLOCK | O_CLOEXEC) != 0) { fds[0] = fds[1] = -1; } });
        if (fds[0] < 0) { usable = false; return false; }

        std::lock_guard<std::mutex> lock(mtx);
        const ssize_t w = write(fds[1], addr, 1);
        if (w == 1) {
            char c;
            (void)read(fds[0], &c, 1);
            usable = true;
            return true;
        }
        usable = (w < 0 && errno == EFAULT);
        return false;
    }

    // true si l'octet a 'addr' peut etre lu.
    inline bool Readable(const void* addr)
    {
        static const pid_t self = getpid();
        char tmp = 0;
        struct iovec local  = { &tmp, 1 };
        struct iovec remote = { const_cast<void*>(addr), 1 };
        const ssize_t r = process_vm_readv(self, &local, 1, &remote, 1, 0);
        if (r == 1) return true;
        if (r < 0 && errno == EFAULT) return false;

        // process_vm_readv refuse (EPERM / ENOSYS selon l'appareil) : test par pipe.
        bool usable = false;
        const bool ok = ReadableByPipe(addr, usable);
        if (usable) return ok;

        // Dernier recours (aucun test fiable disponible) : mincore.
        unsigned char vec = 0;
        const uintptr_t page = reinterpret_cast<uintptr_t>(addr) & ~(static_cast<uintptr_t>(getpagesize()) - 1);
        return mincore(reinterpret_cast<void*>(page), 1, &vec) == 0;
    }

    // Garde de lecture sequentielle : memorise la fin de la zone deja verifiee (alignee sur une page).
    struct ReadGuard
    {
        uintptr_t verifiedEnd = 0;

        // true si [p, p + len) est entierement lisible.
        bool Ensure(const uint8_t* p, size_t len)
        {
            const uintptr_t pageSize = static_cast<uintptr_t>(getpagesize());
            const uintptr_t need = reinterpret_cast<uintptr_t>(p) + len;
            if (need <= verifiedEnd) return true;

            uintptr_t page = reinterpret_cast<uintptr_t>(p) & ~(pageSize - 1);
            if (verifiedEnd > page) page = verifiedEnd;
            const uintptr_t endPage = (need + pageSize - 1) & ~(pageSize - 1);

            while (page < endPage) {
                if (!Readable(reinterpret_cast<const void*>(page))) return false;
                page += pageSize;
                verifiedEnd = page;
            }
            return true;
        }
    };
}
