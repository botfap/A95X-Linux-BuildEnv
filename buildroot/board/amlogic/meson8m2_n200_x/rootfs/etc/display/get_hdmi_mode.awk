#!/usr/bin/awk
BEGIN{
    output_mode = "720p"
}
/\*/{
    output_mode = $0
    sub(/\*/, "", output_mode)
}
END{
    print output_mode
}
