# Common Makefile patterns
# Include in project Makefile: include path/to/cpm/lib/make/common.mk

# LOG=1 enables automatic tee to .tmp/<target>.log
LOG ?= 1
ifdef LOG
ifneq ($(LOG),0)
define log_footer
	@echo "  >>> log: .tmp/$@.log"
endef
SHELL := /bin/bash
.SHELLFLAGS := -o pipefail -c
$(shell mkdir -p .tmp)
endif
endif

# Help target with awk parsing
.PHONY: help
help: ## Show this help
	@awk 'BEGIN {FS = ":.*##"; printf "\nUsage: make \033[36m<target>\033[0m\n"} \
		/^[a-zA-Z_-]+:.*?##/ { printf "  \033[36m%-20s\033[0m %s\n", $$1, $$2 } \
		/^##@/ { printf "\n\033[1m%s\033[0m\n", substr($$0, 5) }' $(MAKEFILE_LIST)
