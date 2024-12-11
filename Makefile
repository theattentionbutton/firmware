.PHONY: all icons midis build_html build_project

# Grouped build tasks for convenience
all: icons midis build_html build_project

# Build HTML and assets for the captive portal page
build_html:
	node captive-portal-page/build.mjs ./captive-portal-page

# Build the main project using PlatformIO
build_project:
	pio run

# Generate icons from source (triggered if needed)
icons:
	deno run -A scripts/make-icons.ts -- ./src/icons.h

midis:
	deno run -A scripts/make-midis.ts -- ./src/midis.h