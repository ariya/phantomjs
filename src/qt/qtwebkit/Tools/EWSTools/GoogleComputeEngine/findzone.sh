if [[ $# -ne 1 ]]; then
    echo "Usage: findzone.sh PROJECT"
fi

echo $(gcutil --project=$1 listzones | grep UP | awk '{print $2}' | sort | tail -1)
