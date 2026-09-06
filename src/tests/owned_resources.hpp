#pragma once

#include <csignal>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Resource ownership for tests that create real processes and descriptors, so
// that cleanup happens on every path out of a test, including a failed
// assertion.
namespace devdock::test_support {

    // Owns a forked child. On destruction the child is signalled if it is still
    // running and then reaped, so no test can leave a stray process behind or
    // block forever waiting for one that never exits.
    class OwnedChild {
    public:
        explicit OwnedChild(pid_t pid) : pid_{pid} {}

        OwnedChild(const OwnedChild&) = delete;
        OwnedChild& operator=(const OwnedChild&) = delete;
        OwnedChild(OwnedChild&&) = delete;
        OwnedChild& operator=(OwnedChild&&) = delete;

        ~OwnedChild() {
            if (pid_ <= 0) {
                return;
            }

            // Harmless when the child has already exited or is a zombie: the
            // signal is dropped and waitpid still collects it.
            ::kill(pid_, SIGKILL);

            ::waitpid(pid_, nullptr, 0);
        }

        [[nodiscard]] pid_t pid() const {
            return pid_;
        }

    private:
        pid_t pid_;
    };

    class OwnedDescriptor {
    public:
        explicit OwnedDescriptor(int descriptor) : descriptor_{descriptor} {}

        OwnedDescriptor(const OwnedDescriptor&) = delete;
        OwnedDescriptor& operator=(const OwnedDescriptor&) = delete;
        OwnedDescriptor(OwnedDescriptor&&) = delete;
        OwnedDescriptor& operator=(OwnedDescriptor&&) = delete;

        ~OwnedDescriptor() {
            if (descriptor_ >= 0) {
                ::close(descriptor_);
            }
        }

        [[nodiscard]] int get() const {
            return descriptor_;
        }

    private:
        int descriptor_;
    };

}
