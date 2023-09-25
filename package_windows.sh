#!/bin/bash
echo "Building Mumble plugin for Windows"
cp build/Release/plugin.dll plugin.dll
cp mumble/manifest.xml manifest.xml
# remove <plugin os="linux" arch="x64">libplugin.so</plugin> from manifest.xml
sed -i '/<plugin os="linux" arch="x64">libplugin.so<\/plugin>/d' manifest.xml
zip -MM -r factorio.mumble_plugin plugin.dll manifest.xml
mv factorio.mumble_plugin build/factorio.mumble_plugin
rm plugin.dll manifest.xml
echo "Done -> build/factorio.mumble_plugin"
