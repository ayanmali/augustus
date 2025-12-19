#include <iostream>
#include <libvirt/libvirt.h>
#include <string>
#include <vector>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#define MB_SIZE 1024

enum DomainType {
    QEMU = 0,
    KVM = 1,
    // Add other domain types here
};

static const std::string domain_type_strings[] = {
    "qemu",
    "kvm",
};

// Can use different virtualization providers (QEMU, KVM, etc.)
class VMManager {
    private:
        DomainType domain_type; // qemu, kvm, etc.
        virConnectPtr conn;
        /**
         * @brief Convert a libvirt domain state code to a human-readable string.
         *
         * Maps libvirt domain state constants (VIR_DOMAIN_*) to a short descriptive
         * string suitable for display.
         *
         * @param state libvirt domain state code (one of the `VIR_DOMAIN_*` constants).
         * @return std::string Human-readable state: `"Running"`, `"Blocked"`, `"Paused"`,
         * `"Shutdown"`, `"Shutoff"`, `"Crashed"`, or `"Unknown"` if the state is unrecognized.
         */
        std::string getStateString(unsigned char state) const {
            switch(state) {
                case VIR_DOMAIN_RUNNING: return "Running";
                case VIR_DOMAIN_BLOCKED: return "Blocked";
                case VIR_DOMAIN_PAUSED: return "Paused";
                case VIR_DOMAIN_SHUTDOWN: return "Shutdown";
                case VIR_DOMAIN_SHUTOFF: return "Shutoff";
                case VIR_DOMAIN_CRASHED: return "Crashed";
                default: return "Unknown";
            }
        }

        /**
         * @brief Finds the QEMU emulator binary path.
         * 
         * Searches common installation locations for qemu-system-x86_64.
         * @return std::string Path to QEMU binary, or empty string if not found.
         */
        std::string findQEMUPath() const {
            // Common QEMU paths
            std::vector<std::string> paths = {
                "/opt/homebrew/bin/qemu-system-x86_64",  // Homebrew (Apple Silicon)
                "/usr/local/bin/qemu-system-x86_64",      // Homebrew (Intel Mac)
                "/usr/bin/qemu-system-x86_64",            // Linux standard
            };
            
            for (const auto& path : paths) {
                struct stat buffer;
                if (stat(path.c_str(), &buffer) == 0 && S_ISREG(buffer.st_mode)) {
                    // Check if executable
                    if (access(path.c_str(), X_OK) == 0) {
                        return path;
                    }
                }
            }
            
            // Try to find via which/command
            FILE* pipe = popen("which qemu-system-x86_64 2>/dev/null", "r");
            if (pipe) {
                char buffer[512];
                if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    std::string result(buffer);
                    pclose(pipe);
                    // Remove trailing newline
                    if (!result.empty() && result.back() == '\n') {
                        result.pop_back();
                    }
                    if (!result.empty()) {
                        return result;
                    }
                } else {
                    pclose(pipe);
                }
            }
            
            return ""; // Not found
        }

    public:
        /**
        * @brief Constructs a VMManager and initializes the libvirt connection handle.
        *
        * Initializes the internal connection pointer to null so the manager starts
        * without an active libvirt connection.
        */
        VMManager(DomainType domain_type) : conn(nullptr), domain_type(domain_type) {}
        /**
        * @brief Releases VMManager resources.
        *
        * Closes the libvirt connection held by this VMManager instance if one exists.
        */
        ~VMManager() { if (conn) virConnectClose(conn); }
        /**
         * @brief Establishes a connection to a libvirt daemon at the specified URI.
         *
         * @param uri Connection URI for libvirt (for example, "qemu:///system").
         * @return true if the connection was opened successfully, false otherwise.
         */
        bool connect(const std::string& uri) {
            conn = virConnectOpen(uri.c_str());
            if (!conn) {
                std::cerr << "\nFailed to connect to libvirt (URI: " << uri << ")" << std::endl;
                std::cerr << "\nNote: On macOS, you may need to start the libvirt daemon:" << std::endl;
                std::cerr << "  brew services start libvirt" << std::endl;
                std::cerr << "Or try connecting to a different URI (e.g., 'qemu:///session')" << std::endl;
                return false;
            }
            std::cout << "Connected to libvirt\n";
            return true;
        }

        /**
         * @brief Defines a minimal KVM domain using the provided name, memory size, and vCPU count.
         *
         * Creates and registers a domain definition (but does not start the domain). The provided
         * name is used as the domain name and as the base filename for the VM disk image.
         *
         * @param name Domain name and base filename for the VM's disk image.
         * @param memory Memory size in MiB.
         * @param vcpus Number of virtual CPUs.
         * @return virDomainPtr Pointer to the defined domain on success, `nullptr` on failure.
         */
        virDomainPtr createVM(const std::string& name, int memory, int vcpus) {
            // Find QEMU binary path
            std::string qemu_path = findQEMUPath();
            if (qemu_path.empty()) {
                std::cerr << "Error: QEMU binary not found. Please install QEMU:" << std::endl;
                std::cerr << "  macOS: brew install qemu" << std::endl;
                std::cerr << "  Linux: apt-get install qemu-system-x86 (Debian/Ubuntu)" << std::endl;
                return nullptr;
            }
            
            // Determine disk image path based on platform
            std::string disk_path;
            const char* home = std::getenv("HOME");
            if (home) {
                // macOS: use user's libvirt directory
                disk_path = std::string(home) + "/.local/share/libvirt/images/" + name + ".qcow2";
            } else {
                // Fallback to Linux standard path
                disk_path = "/var/lib/libvirt/images/" + name + ".qcow2";
            }
            
            // Minimal VM XML configuration
            std::string xml = 
            "<domain type='" + domain_type_strings[domain_type] + "'>"
            "  <name>" + name + "</name>"
            "  <memory unit='MiB'>" + std::to_string(memory) + "</memory>"
            "  <vcpu>" + std::to_string(vcpus) + "</vcpu>"
            "  <os>"
            "    <type arch='x86_64'>hvm</type>"
            "    <boot dev='hd'/>"
            "  </os>"
            "  <features>"
            "    <acpi/>"
            "    <apic/>"
            "  </features>"
            "  <devices>"
            "    <emulator>" + qemu_path + "</emulator>"
            "    <disk type='file' device='disk'>"
            "      <driver name='qemu' type='qcow2'/>"
            "      <source file='" + disk_path + "'/>"
            "      <target dev='vda' bus='virtio'/>"
            "    </disk>"
            "    <interface type='network'>"
            "      <source network='default'/>"
            "      <model type='virtio'/>"
            "    </interface>"
            "    <console type='pty'/>"
            "    <graphics type='vnc' port='-1'/>"
            "  </devices>"
            "</domain>";

            virDomainPtr dom = virDomainDefineXML(conn, xml.c_str());
            if (!dom) {
                std::cerr << "Failed to define domain\n";
                return nullptr;
            }
            
            std::cout << "VM '" << name << "' defined successfully\n";
            return dom;
        }

        /**
         * @brief Starts the given libvirt domain.
         *
         * @param vm Domain handle to start.
         * @return true if the domain was started successfully, false otherwise.
         */
        bool startVM(virDomainPtr vm) {
            if (virDomainCreate(vm) < 0) {
                std::cerr << "Failed to start domain\n";
                return false;
            }
            std::cout << "VM started successfully\n";
            return true;
        }

        /**
         * Stops the specified virtual machine.
         *
         * @param vm Libvirt domain handle representing the VM to stop.
         * @return `true` if the VM was stopped successfully, `false` otherwise.
         */
        bool stopVM(virDomainPtr vm) {
            if (virDomainShutdown(vm) < 0) {
                std::cerr << "Failed to stop domain\n";
                return false;
            }
            std::cout << "VM '" << virDomainGetName(vm) << "' stopped successfully\n";
            return true;
        }

        /**
         * @brief Permanently destroys the given libvirt domain.
         *
         * @param vm Pointer to the libvirt domain to be destroyed.
         * @return bool `true` if the domain was destroyed successfully, `false` otherwise.
         */
        bool destroyVM(virDomainPtr vm) {
            if (virDomainDestroy(vm) < 0) {
                std::cerr << "Failed to destroy VM\n";
                return false;
            }
            virDomainFree(vm);
            std::cout << "VM '" << virDomainGetName(vm) << "' destroyed successfully\n";
            return true;
        }

        /**
         * @brief Undefines the given libvirt domain. The domain must be shut off to be undefined.
         *
         * @param vm Pointer to the libvirt domain to be undefined.
         * @return bool `true` if the domain was undefined successfully, `false` otherwise.
         */
        bool undefineVM(virDomainPtr vm) {
            if (virDomainUndefine(vm) < 0) {
                std::cerr << "Failed to undefine VM\n";
                return false;
            }
            std::cout << "VM '" << virDomainGetName(vm) << "' undefined successfully\n";
            return true;
        }

        /**
         * @brief Finds a libvirt domain by its name.
         *
         * @param name Domain name to look up.
         * @return virDomainPtr Pointer to the domain if found, `nullptr` otherwise.
         *
         * Caller is responsible for freeing the returned domain handle with `virDomainFree`.
         */
        virDomainPtr lookupVM(const std::string& name) {
            virDomainPtr vm = virDomainLookupByName(conn, name.c_str());
            if (!vm) {
                std::cerr << "VM '" << name << "' not found\n";
                return nullptr;
            }
            std::cout << "VM '" << name << "' found\n";
            return vm;
        }

        /**
         * @brief Lists all libvirt domains known to the current connection and prints their basic info.
         *
         * Queries libvirt for all domains on the active connection and writes a summary to standard output:
         * the total number of domains followed by each domain's name, human-readable state, and memory size in MB.
         * If domain enumeration fails, an error message is written to standard error.
         *
         * This function releases each returned domain handle and the domains array before returning.
         */
        void listVMs() {
            virDomainPtr *domains;
            int num = virConnectListAllDomains(conn, &domains, 0);
            
            if (num < 0) {
                std::cerr << "Failed to list domains\n";
                return;
            }

            std::cout << "Found " << num << " domains:\n";
            for (int i = 0; i < num; i++) {
                const char* name = virDomainGetName(domains[i]);
                virDomainInfo info;
                virDomainGetInfo(domains[i], &info);
                
                std::cout << "  - " << name 
                        << " (State: " << getStateString(info.state) 
                        << ", Memory: " << info.memory / MB_SIZE << "MB)\n";
                
                virDomainFree(domains[i]);
            }
            //free(domains);
        }
        /**
         * @brief Prints the domain's name and human-readable state to standard output.
         *
         * @param vm Pointer to a libvirt domain object whose name and state will be printed.
         *           Must be non-null; behavior is undefined if `vm` is null.
         */
        void getVMState(virDomainPtr vm) {
            const char* name = virDomainGetName(vm);
            virDomainInfo info;
            if (virDomainGetInfo(vm, &info) == 0) {
                std::cout << "VM '" << name << "' state: " << getStateString(info.state) << "\n";
            } else {
                std::cerr << "Failed to get VM state\n";
            }
        }
};

/**
 * @brief Program entry demonstrating basic VMManager usage.
 *
 * Connects to libvirt, lists existing virtual machines, and attempts to define
 * a sample VM named "test-vm" (the created domain is freed before exit).
 *
 * @return int 0 on success, 1 if connecting to libvirt failed.
 */
// int main() {
//     VMManager manager(QEMU);
    
//     // Connect to QEMU/KVM
//     if (!manager.connect("qemu:///system")) {
//         std::cerr << "Failed to connect to libvirt\n";
//         return 1;
//     }

//     // List existing VMs
//     std::cout << "\n=== Existing VMs ===\n";
//     manager.listVMs();

//     // Example: Create and start a VM
//     std::string vmName = "test-vm";
//     virDomainPtr vm = manager.createVM(vmName, "qemu", 1024, 2); // 1GB RAM, 2 vCPUs
    
//     if (vm) {
//         // Note: You'd need to create the disk image first
//         // qemu-img create -f qcow2 /var/lib/libvirt/images/test-vm.qcow2 10G
        
//         // manager.startVM(vm);
//         // ... do work ...
//         // manager.stopVM(vm);
        
//         virDomainFree(vm);
//     }

//     return 0;
// }