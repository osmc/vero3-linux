#!/bin/bash

echo -e "Enter Debian package version"
read tag

echo -e "Enter platform, vero5 or vero3"
read platform

git tag -a $platform-4.9.269-$tag-osmc -m "OSMC kernel release"
echo -e "Tag created successfully"
git push origin $platform-4.9.269-$tag-osmc
