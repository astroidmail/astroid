CLOSURE_ARGS    ?= --warning_level=DEFAULT
CLOSURECOMPILER ?= closure-compiler

ASTROID_JS      := astroid.js email-css.js stub.js main.js
CLOSURE_JS_ARGS := $(addprefix --js , $(ASTROID_JS))
BUNDLE_JS       := bundle.js
COMPILED_JS     := bundle.min.js

default: build

vendor/preact.js:
	@curl "https://unpkg.com/preact@8.2.5/dist/preact.js" > vendor/preact.js

vendor/ramda.js:
	curl "https://unpkg.com/ramda@0.24.1/dist/ramda.js" > vendor/ramda.js

vendor/hyperx.js:
	curl "https://wzrd.in/standalone/hyperx@2.3.0" > vendor/hyperx.js

$(BUNDLE_JS): vendor/hyperx.js vendor/ramda.js vendor/preact.js
	rm -f $(BUNDLE_JS)
	for f in vendor/*.js; do cat $$f >> $(BUNDLE_JS); echo '' >> $(BUNDLE_JS); done
	cat $(ASTROID_JS) >> $(BUNDLE_JS)

$(COMPILED_JS): $(BUNDLE_JS)
	$(CLOSURECOMPILER) --compilation_level SIMPLE \
		--language_in ECMASCRIPT_NEXT \
		--language_out ECMASCRIPT_2015 \
		--js_output_file $(COMPILED_JS) \
		$(CLOSURE_ARGS) --js $(BUNDLE_JS)

build: $(COMPILED_JS)
	@echo "Compilation finished."
	@printf "Before: %7s bytes\n" `wc -c $(BUNDLE_JS) | cut -d' ' -f1`
	@printf "After : %7s bytes\n" `wc -c $(COMPILED_JS) | cut -d' ' -f1`

build-no-compile: $(BUNDLE_JS)
	cp $(BUNDLE_JS) $(COMPILED_JS)

clean:
	rm -f $(BUNDLE_JS) $(COMPILED_JS)

dist-clean: clean
	rm -f vendor/*.js

