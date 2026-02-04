#!/bin/bash
# Continuous Documentation Dashboard (CDD) - Agentic TDD Edition

# --- Palette ---
C_CYAN="\033[1;36m"
C_MAGENTA="\033[1;35m"
C_GREEN="\033[1;32m"
C_RED="\033[1;31m"
C_YELLOW="\033[1;33m"
C_BLUE="\033[1;34m"
C_WHITE="\033[1;37m"
C_DIM="\033[2m"
C_RESET="\033[0m"

# --- Config ---
# MAX_DONE_FEATURES will be calculated dynamically

# --- Icons ---
ICON_BRAIN="🧠"
ICON_TEST="🧪"
ICON_CHECK="✔ "
ICON_WORK="🔨"
ICON_PENDING="⏳"
ICON_GIT="DATA"

# Hide cursor for a clean refresh, and set a trap to restore it on exit
tput civis
trap "tput cnorm; exit" INT TERM

while true; do
    # Initialize an empty output buffer for this frame
    output_buffer=""

    # --- Build Header ---
    output_buffer+="${C_CYAN}======================================================${C_RESET}\033[K\n"
    output_buffer+="${C_CYAN}   🚀 PROJECT MONITOR  ::  $(date +'%H:%M:%S')${C_RESET}\033[K\n"
    output_buffer+="${C_CYAN}======================================================${C_RESET}\033[K\n"

    # --- Build Workspace Context ---
    output_buffer+="\n\033[K" # Blank line
    output_buffer+="${C_MAGENTA}=== ${ICON_BRAIN}  WORKSPACE CONTEXT (Git Status) ===${C_RESET}\033[K\n"
    
    git_status_porcelain=$(git status --porcelain)
    if [ -z "$git_status_porcelain" ]; then
        output_buffer+="   ${C_GREEN}${ICON_CHECK} Clean State${C_RESET} ${C_DIM}(Ready for next task)${C_RESET}\033[K\n"
    else
        output_buffer+="   ${C_YELLOW}${ICON_WORK}  Work in Progress:${C_RESET}\033[K\n"
        # Process each line of git status output
        while IFS= read -r line; do
            output_buffer+="      ${line}\033[K\n"
        done <<< "$git_status_porcelain"
    fi

    # --- Build Feature Queue ---
    output_buffer+="\n\033[K" # Blank line
    output_buffer+="${C_CYAN}=== 📜 FEATURE QUEUE (features/*.md) ===${C_RESET}\033[K\n"
    
    if [ -d "features" ]; then
        done_features=()
        testing_features=()
        todo_features=()

        # Determine feature status
        for f in features/*.md; do
            [ -e "$f" ] || continue
            fname=$(basename "$f")
            complete_commit=$(git log -1 --grep="\[Complete features/$fname\]" --format=%H 2>/dev/null)
            test_commit=$(git log -1 --grep="\[Ready for HIL Test features/$fname\]" --format=%H 2>/dev/null)
            complete_timestamp=0; [ -n "$complete_commit" ] && complete_timestamp=$(git show -s --format=%ct $complete_commit 2>/dev/null)
            test_timestamp=0; [ -n "$test_commit" ] && test_timestamp=$(git show -s --format=%ct $test_commit 2>/dev/null)
            file_timestamp=$(git log -1 --format=%ct -- "$f" 2>/dev/null)

            if [ "$complete_timestamp" -gt "$test_timestamp" ]; then
                if [ "$file_timestamp" -le "$complete_timestamp" ]; then done_features+=("$complete_timestamp:$fname"); else todo_features+=("$fname"); fi
            elif [ "$test_timestamp" -gt 0 ]; then
                if [ "$file_timestamp" -le "$test_timestamp" ]; then testing_features+=("$fname"); else todo_features+=("$fname"); fi
            else
                todo_features+=("$fname")
            fi
        done

        # --- Dynamic MAX_DONE_FEATURES Calculation ---
        terminal_height=$(tput lines)
        calculated_current_lines_used=0
        ((calculated_current_lines_used+=4)) # Header + blank line
        ((calculated_current_lines_used+=1)) # Workspace header
        GIT_STATUS_LINES=$(echo "$git_status_porcelain" | sed '/^\s*$/d' | wc -l | xargs)
        if [ "$GIT_STATUS_LINES" -eq 0 ]; then ((calculated_current_lines_used+=1)); else ((calculated_current_lines_used+=($GIT_STATUS_LINES + 1))); fi
        ((calculated_current_lines_used+=2)) # Blank line + Feature header
        ((calculated_current_lines_used+=${#testing_features[@]}))
        ((calculated_current_lines_used+=${#todo_features[@]}))
        ((calculated_current_lines_used+=2)) # Blank line + LATEST SAVE header
        ((calculated_current_lines_used+=1)) # Git log output
        ((calculated_current_lines_used+=2)) # Blank line + TEST STATUS header
        ((calculated_current_lines_used+=1)) # Test status output
        ((calculated_current_lines_used+=2)) # Blank line + Footer
        MAX_DONE_FEATURES=$((terminal_height - calculated_current_lines_used - 1)) 
        if [ "$MAX_DONE_FEATURES" -lt 0 ]; then MAX_DONE_FEATURES=0; fi

        # Append features to buffer
        for fname in "${testing_features[@]}"; do output_buffer+="   ${C_BLUE}[TESTING]${C_RESET} $fname\033[K\n"; done
        for fname in "${todo_features[@]}"; do output_buffer+="   ${C_YELLOW}[TODO]${C_RESET}   $fname\033[K\n"; done

        IFS=$'\n' sorted_done=($(sort -rn <<<"${done_features[*]}")); unset IFS
        for ((i=0; i<${#sorted_done[@]}; i++)); do
            if [ "$i" -lt "$MAX_DONE_FEATURES" ]; then
                fname=${sorted_done[$i]#*:}
                output_buffer+="   ${C_GREEN}[DONE]${C_RESET}   $fname\033[K\n"
            fi
        done
        if [ "${#sorted_done[@]}" -gt "$MAX_DONE_FEATURES" ]; then
            output_buffer+="   ${C_DIM}...and $((${#sorted_done[@]} - MAX_DONE_FEATURES)) more done features.${C_RESET}\033[K\n"
        fi
    else
        output_buffer+="${C_RED}No features/ directory found.${C_RESET}\033[K\n"
    fi

    # --- Build Footer ---
    output_buffer+="\n\033[K" 
    output_buffer+="${C_BLUE}=== 💾 LATEST SAVE (Last Commit) ===${C_RESET}\033[K\n"
    last_commit=$(git log -1 --format="   %C(green)%h%Creset %s %C(dim white)(%cr)%Creset" 2>/dev/null)
    if [ -n "$last_commit" ]; then output_buffer+="${last_commit}\033[K\n"; else output_buffer+="   ${C_DIM}No commits yet.${C_RESET}\033[K\n"; fi

    output_buffer+="\n\033[K"
    output_buffer+="${C_MAGENTA}=== ${ICON_TEST}  TEST STATUS ===${C_RESET}\033[K\n"
    if [ -f ".pio/testing/last_summary.json" ]; then
        if grep -q "\"succeeded\": false" .pio/testing/last_summary.json; then
             output_buffer+="   ${C_RED}✖  FAIL${C_RESET} - Logic Broken\033[K\n"
        else
             output_buffer+="   ${C_GREEN}✔  PASS${C_RESET} - Systems Nominal\033[K\n"
        fi
    else
        output_buffer+="   ${C_DIM}?  No Test History${C_RESET}\033[K\n"
    fi

    output_buffer+="\n\033[K"
    output_buffer+="${C_DIM}(Press Ctrl+C to stop monitoring)${C_RESET}\033[K\n"

    # --- Render Frame ---
    printf "\033[1;1H"
    printf "%b" "$output_buffer"
    
    sleep 5
done