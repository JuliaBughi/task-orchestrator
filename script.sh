#!/bin/bash

run_commands() {
    for command in "$@"; do
        echo "Executar commando: $command"
        eval "$command"
        if [ $? -ne 0 ]; then
            echo "Erro ao executar o commando: $command"
        fi
    done
}

commands=(
    "./bin/client execute 0 -u \"ls -l\""
    "./bin/client execute 1 -p \"cat /etc/passwd | grep root\""
    "./bin/client execute 1080 -u \"dd if=/dev/zero of=test6m_file.txt bs=6M count=1000\""
    "./bin/client execute 0 -u \"pwd\""
    "./bin/client execute 1080 -u \"dd if=/dev/zero of=test6m2_file.txt bs=6M count=1000\""
    "./bin/client execute 3000 -u \"cat test_file.txt\""
    "./bin/client execute 0 -u \"echo 'Hello, World!'\""
    "./bin/client execute 3000 -u \"cat test_file.txt\""
    "./bin/client execute 990 -u \"dd if=/dev/zero of=test5m_file.txt bs=5M count=1000\""
    "./bin/client execute 1 -p \"echo 'frAse MUIto graaaaaaaaaaaaaaaaaaaaaaAAAAAAAAAAaaaAAaaAAAAaande' | tr '[:lower:]' '[:upper:]'\""
    "./bin/client execute 0 -p \"ls /etc | wc -l\""
    "./bin/client execute 0 -p \"grep -v ˆ# /etc/passwd | cut -f7 -d: | uniq | wc -l\""
    "./bin/client execute 0 -u \"pwd\""
    "./bin/client execute 600 -u \"dd if=/dev/zero of=test3m_file.txt bs=3M count=1000\""
    "./bin/client execute 0 -p \"ls /etc | wc -l\""
)

run_commands "${commands[@]}"