#!/bin/bash

# Require a commit message
if [ $# -eq 0 ]; then
    echo "Usage: $0 \"commit message\""
    exit 1
fi

COMMIT_MESSAGE="$*"
USERNAME="gabekanzelmeyer"
PASSWORD_FILE="/home/gabe/.github_pw"

# Read password from file
if [ ! -f "$PASSWORD_FILE" ]; then
    echo "Password file not found: $PASSWORD_FILE"
    exit 1
fi

PASSWORD=$(cat "$PASSWORD_FILE")

# Make sure the password isn't empty
if [ -z "$PASSWORD" ]; then
    echo "Password file is empty"
    exit 1
fi

echo "Adding files..."
git add -A || exit 1

echo "Committing..."
git commit -m "$COMMIT_MESSAGE" || exit 1

# Temporary askpass helper
ASKPASS=$(mktemp)
chmod 700 "$ASKPASS"

cat > "$ASKPASS" <<EOF
#!/bin/sh
case "\$1" in
    *Username*) echo "$USERNAME" ;;
    *Password*) echo "$PASSWORD" ;;
esac
EOF

GIT_ASKPASS="$ASKPASS" \
GIT_TERMINAL_PROMPT=0 \
git push

RESULT=$?

rm -f "$ASKPASS"

exit $RESULT
