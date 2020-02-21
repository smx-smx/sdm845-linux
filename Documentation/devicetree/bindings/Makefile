# SPDX-License-Identifier: GPL-2.0
DT_DOC_CHECKER ?= dt-doc-validate
DT_EXTRACT_EX ?= dt-extract-example
DT_MK_SCHEMA ?= dt-mk-schema
DT_MK_SCHEMA_FLAGS := $(if $(DT_SCHEMA_FILES), -u)

quiet_cmd_chk_binding = CHKDT   $(patsubst $(srctree)/%,%,$<)
      cmd_chk_binding = $(DT_DOC_CHECKER) -u $(srctree)/$(src) $< ; \
                        $(DT_EXTRACT_EX) $< > $@

$(obj)/%.example.dts: $(src)/%.yaml FORCE
	$(call if_changed,chk_binding)

quiet_cmd_mk_schema = SCHEMA  $@
      cmd_mk_schema = $(DT_MK_SCHEMA) $(2) -o $@ $(real-prereqs)

DT_DOCS = $(addprefix $(src)/, \
	$(shell \
	cd $(srctree)/$(src) && \
	find * \( -name '*.yaml' ! \
		-name 'processed-schema-*' ! \
		-name '*.example.dt.yaml' \) \
	))

DT_SCHEMA_FILES ?= $(DT_DOCS)

ifeq ($(CHECK_DTBS),)
extra-y += $(patsubst $(src)/%.yaml,%.example.dts, $(DT_SCHEMA_FILES))
extra-y += $(patsubst $(src)/%.yaml,%.example.dt.yaml, $(DT_SCHEMA_FILES))
endif

$(obj)/processed-schema-examples.yaml: $(DT_DOCS) FORCE
	$(call if_changed,mk_schema)

$(obj)/processed-schema-dtbs.yaml: $(DT_SCHEMA_FILES) FORCE
	$(call if_changed,mk_schema, $(DT_MK_SCHEMA_FLAGS))

extra-y += processed-schema-examples.yaml processed-schema-dtbs.yaml
