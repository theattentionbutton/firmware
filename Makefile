build-html:
	node captive-portal-page/build.mjs ./captive-portal-page

build:
	pio run -t size

icons: .FORCE
	python3 icons/convert.py ./src/icons.h

.FORCE: