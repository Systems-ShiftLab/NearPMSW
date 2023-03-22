awk -F ' ' '$1 ~ /time/ {sum += $3} END {print sum}' out
awk -F ' ' '$1 ~ /pagecnt/ {sum += $2} END {print sum}' out
awk -F ' ' '$1 ~ /timecp/ {sum += $3} END {print sum}' out
