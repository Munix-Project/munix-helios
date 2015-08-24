
#!/bin/bash
TOOLS=./munix_helios_toolchain/util
echo "Installing Helios..."
$TOOLS/install_helios.sh

echo "Launching Helios..."
$TOOLS/launch_qemu.sh
