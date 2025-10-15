
static void usbEventCallback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (event_base == ARDUINO_USB_EVENTS) {
    arduino_usb_event_data_t *data = (arduino_usb_event_data_t *)event_data;
    switch (event_id) {
      case ARDUINO_USB_STARTED_EVENT: 
        Serial.println("USB PLUGGED");
        usbReady = true;
        mscReady =true;
        break;
      case ARDUINO_USB_STOPPED_EVENT: 
        Serial.println("USB UNPLUGGED");
        usbReady = false;
        mscReady = false;
        break;
      case ARDUINO_USB_SUSPEND_EVENT: 
        Serial.printf("USB SUSPENDED: remote_wakeup_en: %u\n", data->suspend.remote_wakeup_en); 
        break;
      case ARDUINO_USB_RESUME_EVENT:  
        Serial.println("USB RESUMED"); 
        break;
      default: 
        break;
    }
  }
}
 

 


static int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
    if (lba >= MSC_SECTOR_COUNT || bufsize == 0) {
        return 0;
    }

    uint8_t* dst = (uint8_t*)buffer;
    uint32_t bytes_remaining = bufsize;

    while (bytes_remaining > 0) {
        uint32_t sector = lba;
        uint32_t sector_offset = offset;
        uint32_t bytes_in_this_sector = min(bytes_remaining, MSC_SECTOR_SIZE - sector_offset);

        // Leer el sector completo en un buffer temporal
        uint8_t sector_buffer[512];
        if (sd_readsector(sector, sector_buffer, 1) != 1) {
            USBSerial.printf("Error leyendo sector %u\n", sector);
            return bufsize - bytes_remaining; // devolver lo leído hasta ahora
        }

        // Copiar la porción solicitada
        memcpy(dst, sector_buffer + sector_offset, bytes_in_this_sector);

        // Avanzar
        dst += bytes_in_this_sector;
        bytes_remaining -= bytes_in_this_sector;
        lba++;
        offset = 0; // después del primer sector, offset = 0
    }

    return bufsize;
}

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
    if (lba >= MSC_SECTOR_COUNT || bufsize == 0) {
        return 0;
    }

    uint8_t* src = buffer;
    uint32_t bytes_remaining = bufsize;

    while (bytes_remaining > 0) {
        uint32_t sector = lba;
        uint32_t sector_offset = offset;
        uint32_t bytes_in_this_sector = min(bytes_remaining, MSC_SECTOR_SIZE - sector_offset);

        // Leer el sector actual (para preservar bytes no modificados)
        uint8_t sector_buffer[512];
        if (sd_readsector(sector, sector_buffer, 1) != 1) {
            USBSerial.printf("Error leyendo sector %u para escritura\n", sector);
            return bufsize - bytes_remaining;
        }

        // Modificar la porción solicitada
        memcpy(sector_buffer + sector_offset, src, bytes_in_this_sector);

        // Escribir el sector completo de vuelta
        if (sd_writesector(sector, sector_buffer, 1) != 1) {
            USBSerial.printf("Error escribiendo sector %u\n", sector);
            return bufsize - bytes_remaining;
        }

        // Avanzar
        src += bytes_in_this_sector;
        bytes_remaining -= bytes_in_this_sector;
        lba++;
        offset = 0;
    }

    return bufsize;
}

static bool onStartStop(uint8_t power_condition, bool start, bool load_eject) {
  USBSerial.printf("MSC Start/Stop: power_condition=%d, start=%d, load_eject=%d\n", 
                power_condition, start, load_eject);
  mscReady = start;
  return true;
}
