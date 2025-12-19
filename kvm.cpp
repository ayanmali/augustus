#include <iostream>
#include <libvirt/libvirt.h>
#include <string>
#include <cstdlib>
#define MB_SIZE 1024

// Can use different virtualization providers (QEMU, KVM, etc.)
class VMManager {
    private:
        virConnectPtr conn;
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

    public:
        VMManager() : conn(nullptr) {}
        ~VMManager() { if (conn) virConnectClose(conn); }

        bool connect(const std::string& uri) {
            conn = virConnectOpen(uri.c_str());
            if (!conn) {
                std::cerr << "Failed to connect to libvirt" << std::endl;
                return false;
            }
            std::cout << "Connected to libvirt\n";
            return true;
        }

        virDomainPtr createVM(const std::string& name, const std::string& domain_type, int memory, int vcpus) {
            // Minimal VM XML configuration
            std::string xml = 
            "<domain type='" + domain_type + "'>"
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
            "    <emulator>/usr/bin/qemu-system-x86_64</emulator>"
            "    <disk type='file' device='disk'>"
            "      <driver name='qemu' type='qcow2'/>"
            "      <source file='/var/lib/libvirt/images/" + name + ".qcow2'/>"
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

        bool startVM(virDomainPtr vm) {
            if (virDomainCreate(vm) < 0) {
                std::cerr << "Failed to start domain\n";
                return false;
            }
            std::cout << "VM started successfully\n";
            return true;
        }

        // stop gracefully
        bool stopVM(virDomainPtr vm) {
            if (virDomainDestroy(vm) < 0) {
                std::cerr << "Failed to stop domain\n";
                return false;
            }
            std::cout << "VM '" << virDomainGetName(vm) << "' stopped successfully\n";
            return true;
        }

        // destroy completely
        bool destroyVM(virDomainPtr vm) {
            if (virDomainDestroy(vm) < 0) {
                std::cerr << "Failed to destroy VM\n";
                return false;
            }
            std::cout << "VM '" << virDomainGetName(vm) << "' destroyed successfully\n";
            return true;
        }

        virDomainPtr lookupVM(const std::string& name) {
            virDomainPtr vm = virDomainLookupByName(conn, name.c_str());
            if (!vm) {
                std::cerr << "VM '" << name << "' not found\n";
                return nullptr;
            }
            return vm;
        }

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
            free(domains);
        }
        void getVMState(virDomainPtr vm) {
            std::cout << "VM '" << vm->name << "' state: " << getStateString(vm->state) << "\n";
        }
};

int main() {
    VMManager manager;
    
    // Connect to KVM
    if (!manager.connect()) {
        return 1;
    }

    // List existing VMs
    std::cout << "\n=== Existing VMs ===\n";
    manager.listVMs();

    // Example: Create and start a VM
    std::string vmName = "test-vm";
    virDomainPtr vm = manager.createVM(vmName, "kvm", 1024, 2); // 1GB RAM, 2 vCPUs
    
    if (vm) {
        // Note: You'd need to create the disk image first
        // qemu-img create -f qcow2 /var/lib/libvirt/images/test-vm.qcow2 10G
        
        // manager.startVM(vm);
        // ... do work ...
        // manager.stopVM(vm);
        
        virDomainFree(vm);
    }

    return 0;
}