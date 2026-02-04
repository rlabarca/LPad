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

while true; do
    tput cup 0 0; tput ed
    echo -e "${C_CYAN}======================================================${C_RESET}"
    echo -e "${C_CYAN}   🚀 PROJECT MONITOR  ::  $(date +'%H:%M:%S')${C_RESET}"
    echo -e "${C_CYAN}======================================================${C_RESET}"

    # 1. ACTIVE WORKSPACE (Replaces "Active Beads")
    # This detects if files are modified. If yes, the Agent is working.
    echo -e "\n${C_MAGENTA}=== ${ICON_BRAIN}  WORKSPACE CONTEXT (Git Status) ===${C_RESET}"
    
    if [ -z "$(git status --porcelain)" ]; then
        echo -e "   ${C_GREEN}${ICON_CHECK} Clean State${C_RESET} ${C_DIM}(Ready for next task)${C_RESET}"
    else
        echo -e "   ${C_YELLOW}${ICON_WORK}  Work in Progress:${C_RESET}"
        # List modified files with indentation
        git status --short | sed 's/^/      /'
    fi

# 2. FEATURE TRACKER (The "Source of Truth")
    echo -e "\n${C_CYAN}=== 📜 FEATURE QUEUE (features/*.md) ===${C_RESET}"
    
    if [ -d "features" ]; then
        done_features=()
        testing_features=()
        todo_features=()

        # Determine feature status (done_features populated for sorting later)
        for f in features/*.md; do
            [ -e "$f" ] || continue
            fname=$(basename "$f")
            
            # --- Final, Corrected Logic ---
            
            # Find the latest of each type of status commit
            complete_commit=$(git log -1 --grep="\[Complete features/$fname\]" --format=%H)
            test_commit=$(git log -1 --grep="\[Ready for HIL Test features/$fname\]" --format=%H)

            # Get their timestamps (0 if not found)
            complete_timestamp=0
            [ -n "$complete_commit" ] && complete_timestamp=$(git show -s --format=%ct $complete_commit)

            test_timestamp=0
            [ -n "$test_commit" ] && test_timestamp=$(git show -s --format=%ct $test_commit)
            
            # Get the file's last modification timestamp
            file_timestamp=$(git log -1 --format=%ct -- "$f")

            # Determine the LATEST status and compare
            if [ "$complete_timestamp" -gt "$test_timestamp" ]; then
                # Complete is the latest status
                if [ "$file_timestamp" -le "$complete_timestamp" ]; then
                    done_features+=("$complete_timestamp:$fname")
                else
                    todo_features+=("$fname")
                fi
            elif [ "$test_timestamp" -gt 0 ]; then
                # Testing is the latest status (or they are equal, and we prefer Testing)
                if [ "$file_timestamp" -le "$test_timestamp" ]; then
                    testing_features+=("$fname")
                else
                    todo_features+=("$fname")
                fi
            else
                # No status commits found
                todo_features+=("$fname")
            fi
            # --- End of Final, Corrected Logic ---
        done

        # --- Dynamic MAX_DONE_FEATURES Calculation ---
        terminal_height=$(tput lines)
        
        # Calculate lines used by static parts of the display
        calculated_current_lines_used=0
        calculated_current_lines_used=$((calculated_current_lines_used + 3)) # Top banner (3 lines: ===, 🚀, ===)
        calculated_current_lines_used=$((calculated_current_lines_used + 1)) # Blank line before WORKSPACE CONTEXT
        calculated_current_lines_used=$((calculated_current_lines_used + 1)) # WORKSPACE CONTEXT header
        
        # Add lines for git status output
        GIT_STATUS_LINES=$(git status --short | wc -l | xargs)
        if [ -z "$(git status --porcelain)" ]; then # Clean state
            calculated_current_lines_used=$((calculated_current_lines_used + 1)); # "Clean State" line
        else
            calculated_current_lines_used=$((calculated_current_lines_used + $GIT_STATUS_LINES)); # Actual git status lines
            if [ "$GIT_STATUS_LINES" -gt 0 ]; then calculated_current_lines_used=$((calculated_current_lines_used + 1)); fi # "Work in Progress:" line if there's output
        fi
        
        calculated_current_lines_used=$((calculated_current_lines_used + 1)) # Blank line before FEATURE QUEUE
        calculated_current_lines_used=$((calculated_current_lines_used + 1)) # FEATURE QUEUE header
        calculated_current_lines_used=$((calculated_current_lines_used + ${#testing_features[@]})) # TESTING features
        calculated_current_lines_used=$((calculated_current_lines_used + ${#todo_features[@]}))   # TODO features
        
        calculated_current_lines_used=$((calculated_current_lines_used + 1)) # Blank line before LATEST SAVE
        calculated_current_lines_used=$((calculated_current_lines_used + 1)) # LATEST SAVE header
        calculated_current_lines_used=$((calculated_current_lines_used + 1)) # Git log output (1 line for -1 --format)
        
        calculated_current_lines_used=$((calculated_current_lines_used + 1)) # Blank line before TEST STATUS
        calculated_current_lines_used=$((calculated_current_lines_used + 1)) # TEST STATUS header
        calculated_current_lines_used=$((calculated_current_lines_used + 1)) # Test status output
        
        calculated_current_lines_used=$((calculated_current_lines_used + 1)) # Blank line before footer
        calculated_current_lines_used=$((calculated_current_lines_used + 1)) # Footer "(Press Ctrl+C...)"

        # Calculate MAX_DONE_FEATURES. Subtract 1 for the potential "...and N more done features." line
        MAX_DONE_FEATURES=$((terminal_height - calculated_current_lines_used - 1)) 
        if [ "$MAX_DONE_FEATURES" -lt 0 ]; then MAX_DONE_FEATURES=0; fi # Ensure it's not negative

        # Print all testing and todo features, which are always visible
        for fname in "${testing_features[@]}"; do
            echo -e "   ${C_BLUE}[TESTING]${C_RESET} $fname"
        done
        for fname in "${todo_features[@]}"; do
            echo -e "   ${C_YELLOW}[TODO]${C_RESET}   $fname"
        done

        # Sort done features by timestamp (newest first)
        IFS=$'\n' sorted_done=($(sort -rn <<<"${done_features[*]}"))
        unset IFS

        # Print the latest N done features
        for ((i=0; i<${#sorted_done[@]}; i++)); do
            if [ "$i" -lt "$MAX_DONE_FEATURES" ]; then
                # Strip timestamp for display: ${string#*:} 
                fname=${sorted_done[$i]#*:}
                echo -e "   ${C_GREEN}[DONE]${C_RESET}   $fname"
            fi
        done

        # If there are more done features than the max, show a summary line
        if [ "${#sorted_done[@]}" -gt "$MAX_DONE_FEATURES" ]; then
            echo -e "   ${C_DIM}...and $((${#sorted_done[@]} - MAX_DONE_FEATURES)) more done features.${C_RESET}"
        fi
    else
        echo -e "   ${C_RED}No features/ directory found.${C_RESET}"
    fi

    # 3. LATEST MEMORY (Last Commit)
    echo -e "\n${C_BLUE}=== 💾 LATEST SAVE (Last Commit) ===${C_RESET}"
    # Pretty print the last commit hash, message, and relative time
    git log -1 --format="   %C(green)%h%Creset %s %C(dim white)(%cr)%Creset" 2>/dev/null || echo -e "   ${C_DIM}No commits yet.${C_RESET}"

    # 4. TEST HEALTH
    echo -e "\n${C_MAGENTA}=== ${ICON_TEST}  TEST STATUS ===${C_RESET}"
    if [ -f ".pio/testing/last_summary.json" ]; then
        # Check specifically for failure in the JSON report
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