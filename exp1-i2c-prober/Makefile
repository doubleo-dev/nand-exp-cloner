# APPLICATION
# Application name
APPLICATION = exp1-i2c-prober

# BOARD
# Board name
BOARD ?= esp32s3-devkit

# BOARD_VERSION
# Board variant name
# '' (empty value) if not applicable
BOARD_VERSION ?= esp32s3-devkitc-1-n32r8v

# DEVELHELP
# 0: Enable safety check for production
# 1: Disable safety check for development help
DEVELHELP ?= 1

# QUIET
# 0: Show compiler invocation lines
# 1: Hide compiler invocation lines
QUIET ?= 0

# APPDIR
# Absolute path to the application directory
APPDIR ?= $(CURDIR)

# APPBASE
# Absolute path to the application base directory
#   This should be set in APPDIR/Makefile
#   If not, treat APPBASE as APPDIR/.. (parent directory of the application directory)
APPBASE ?= $(APPDIR)/..

# SRAM with PSRAM(SPI RAM)
#   Espressif ESP32-S3 PSRAM          https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
#   RIOT OS ESP32-S3 'esp_spi_ram'    https://doc.riot-os.org/group__cpu__esp32__esp32s3.html#esp32_embedded_flash_ram_esp32s3
#   RIOT OS ESP32 Series Heap Impl    https://doc.riot-os.org/group__cpu__esp32.html#esp32_esp_idf_heap_implementation
USEMODULE += esp_spi_ram

# ZTIMER
USEMODULE += ztimer_msec

# I2C
USEMODULE += periph_i2c

include $(APPBASE)/Makefile.include
