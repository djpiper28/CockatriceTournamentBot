find tests/ | grep "\.[ch]$" | xargs -n 1 python3 clean_file.py
find src/ | grep "\.[ch]$" | xargs -n 1 python3 clean_file.py
