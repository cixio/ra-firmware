COMPONENT_NAME=rs41

SRC_FILES = \
  $(PROJECT_SRC_DIR)/src/reedsolomon/reedsolomon.c \
  $(PROJECT_SRC_DIR)/src/rs41/rs41metrology.c \
  $(PROJECT_SRC_DIR)/src/rs41/rs41utils.c \
  $(UNITTEST_SRC_DIR)/../fakes/fake_rs41calibconfig.c

TEST_SRC_FILES = \
  $(UNITTEST_SRC_DIR)/test_rs41.cpp

UNITTEST_EXTRA_INC_PATHS += \
  -I$(PROJECT_SRC_DIR)/src \
  -I$(PROJECT_SRC_DIR)/src/reedsolomon

include $(CPPUTEST_MAKFILE_INFRA)


