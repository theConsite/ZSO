
# Script to kill all processes matching the pattern "Proj"

ipcrm --all=msg

# Find all processes matching the pattern "Proj" and extract their PIDs
pids=$(ps aux | grep 'Proj' | grep -v 'grep' | awk '{print $2}')

# Check if any PIDs were found
if [ -z "$pids" ]; then
    echo "No processes found matching the pattern 'Proj'."
else
    # Loop through each PID and kill the process
    for pid in $pids; do
        echo "Killing process with PID: $pid"
        kill -9 $pid
    done
    echo "All matching processes have been killed."
fi