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

# --- Icons ---
ICON_BRAIN="🧠"
ICON_TEST="🧪"
ICON_CHECK="✔ "
ICON_WORK="🔨"
ICON_PENDING="⏳"

# Restore cursor and exit gracefully
trap "echo; tput cnorm; exit" INT TERM
tput civis

while true; do
    # Clear screen without flicker and move cursor to top
    tput cup 0 0; tput ed

    # --- Header ---
    echo -e "${C_CYAN}======================================================${C_RESET}"
    echo -e "${C_CYAN}   🚀 PROJECT MONITOR  ::  $(date +'%H:%M:%S')${C_RESET}"
    echo -e "${C_CYAN}======================================================${C_RESET}"

    # --- Workspace Context ---
    echo -e "\n${C_MAGENTA}=== ${ICON_BRAIN}  WORKSPACE CONTEXT (Git Status) ===${C_RESET}"
    git_status_porcelain=$(git status --porcelain | grep -v "\.DS_Store")
    if [ -z "$git_status_porcelain" ]; then
        echo -e "   ${C_GREEN}${ICON_CHECK} Clean State${C_RESET} ${C_DIM}(Ready for next task)${C_RESET}"
    else
        echo -e "   ${C_YELLOW}${ICON_WORK}  Work in Progress:${C_RESET}"
        git status --short | grep -v "\.DS_Store" | sed 's/^/      /'
    fi

    # --- Feature Queue ---
    echo -e "\n${C_CYAN}=== 📜 FEATURE QUEUE (features/**/*.md) ===${C_RESET}"
    if [ -d "features" ]; then
        done_features=()
        testing_features=()
        todo_features=()

        # Determine feature status
        for f in features/**/*.md; do
            [ -e "$f" ] || continue
            fname=${f#features/}
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

        # Dynamic height calculation
        terminal_height=$(tput lines)
        calculated_current_lines_used=0
        ((calculated_current_lines_used+=5)) # Header(3) + 2 blank lines
        ((calculated_current_lines_used+=1)) # Workspace header
        GIT_STATUS_LINES=$(echo "$git_status_porcelain" | sed '/^\s*$/d' | wc -l)
        if [ "$GIT_STATUS_LINES" -eq 0 ]; then ((calculated_current_lines_used+=1)); else ((calculated_current_lines_used+=($GIT_STATUS_LINES + 1))); fi
        ((calculated_current_lines_used+=1)) # Feature header
        ((calculated_current_lines_used+=${#testing_features[@]}))
        ((calculated_current_lines_used+=${#todo_features[@]}))
        ((calculated_current_lines_used+=2)) # Blank line + LATEST SAVE header
        ((calculated_current_lines_used+=1)) # Git log output
        ((calculated_current_lines_used+=2)) # Blank line + TEST STATUS header
        ((calculated_current_lines_used+=1)) # Test status output
        ((calculated_current_lines_used+=2)) # Blank line + Footer
        MAX_DONE_FEATURES=$((terminal_height - calculated_current_lines_used - 2)) 
        if [ "$MAX_DONE_FEATURES" -lt 0 ]; then MAX_DONE_FEATURES=0; fi

        # Print features
        for fname in "${testing_features[@]}"; do echo -e "   ${C_BLUE}[TESTING]${C_RESET} $fname"; done
        for fname in "${todo_features[@]}"; do echo -e "   ${C_YELLOW}[TODO]${C_RESET}   $fname"; done

        IFS=$'\n' sorted_done=($(sort -rn <<<"${done_features[*]}")); unset IFS
        for ((i=0; i<${#sorted_done[@]}; i++)); do
            if [ "$i" -lt "$MAX_DONE_FEATURES" ]; then
                fname=${sorted_done[$i]#*:}
                echo -e "   ${C_GREEN}[DONE]${C_RESET}   $fname"
            fi
        done
        if [ "${#sorted_done[@]}" -gt "$MAX_DONE_FEATURES" ]; then
            echo -e "   ${C_DIM}...and $((${#sorted_done[@]} - MAX_DONE_FEATURES)) more done features.${C_RESET}"
        fi
    else
        echo -e "   ${C_RED}No features/ directory found.${C_RESET}"
    fi

    # --- Footer ---
    echo -e "\n${C_BLUE}=== 💾 LATEST SAVE (Last Commit) ===${C_RESET}"
    git log -1 --format="   %C(green)%h%Creset %s %C(dim white)(%cr)%Creset" 2>/dev/null || echo -e "   ${C_DIM}No commits yet.${C_RESET}"

    echo -e "\n${C_MAGENTA}=== ${ICON_TEST}  TEST STATUS ===${C_RESET}"
    if [ -f ".pio/testing/last_summary.json" ]; then
        if grep -q "\"succeeded\": false" .pio/testing/last_summary.json; then
             echo -e "   ${C_RED}✖  FAIL${C_RESET} - Logic Broken"
        else
             echo -e "   ${C_GREEN}✔  PASS${C_RESET} - Systems Nominal"
        fi
    else
        echo -e "   ${C_DIM}?  No Test History${C_RESET}"
    fi

    echo -e "\n${C_DIM}(Press Ctrl+C to stop monitoring)${C_RESET}"
    
    sleep 5
done