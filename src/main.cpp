#include <iostream>
#include "vm.cpp"

int main() {
    std::cout << "Hello, World!\n";

    VMManager manager(QEMU);
    
    // Connect to QEMU/KVM
    // Try system connection first, fall back to session if that fails
    std::string uri = "qemu:///system";
    if (!manager.connect(uri)) {
        std::cerr << "\nTrying session connection instead..." << std::endl;
        uri = "qemu:///session";
        if (!manager.connect(uri)) {
            std::cerr << "\nBoth connection attempts failed." << std::endl;
            std::cerr << "To start libvirt on macOS, run:" << std::endl;
            std::cerr << "  brew services start libvirt" << std::endl;
            return 1;
        }
    }

    // List existing VMs
    std::cout << "\n=== Existing VMs ===\n";
    manager.listVMs();

    // Example: Create and start a VM
    std::string vmName = "test-vm";
    virDomainPtr vm = manager.createVM(vmName, 1024, 2); // 1GB RAM, 2 vCPUs
    
    if (vm) {
        // Note: You'd need to create the disk image first
        // On macOS: qemu-img create -f qcow2 ~/.local/share/libvirt/images/test-vm.qcow2 10G
        // On Linux: qemu-img create -f qcow2 /var/lib/libvirt/images/test-vm.qcow2 10G
        
        // manager.startVM(vm);
        // ... do work ...
        // manager.stopVM(vm);
        
        virDomainFree(vm);
    }

    return 0;
}