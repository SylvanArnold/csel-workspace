set -o errexit
set -o pipefail
set -o nounset
# set -o xtrace

__dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
__root="$(cd "$(dirname "${__dir}")" && pwd)"

targets="""
backtrace
core_dumps
daemon
fibonacci
memory_leaks
process
system_calls
tracing
"""

for i in $targets; do
    mkdir -p $__root/$i/.vscode
    cp $__dir/launch.json $__root/$i/.vscode
    cp $__dir/tasks.json $__root/$i/.vscode
done