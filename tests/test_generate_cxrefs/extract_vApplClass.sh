#!/bin/bash
awk '/^[^[:space:]]/ {
        if (section ~ /vApplClass/) {
            printf "%s", section
            exit
        }
        section = ""  # Börja en ny sektion
    }
    {
        section = section $0 "\n"
    }
' $1
