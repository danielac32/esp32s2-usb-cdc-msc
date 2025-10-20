
const esp_partition_t* partition(){
  return esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, "ffat");
}


static void usbEventCallback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (event_base == ARDUINO_USB_EVENTS) {
    arduino_usb_event_data_t *data = (arduino_usb_event_data_t *)event_data;
    switch (event_id) {
      case ARDUINO_USB_STARTED_EVENT: Serial.println("USB PLUGGED"); break;
      case ARDUINO_USB_STOPPED_EVENT: Serial.println("USB UNPLUGGED"); break;
      case ARDUINO_USB_SUSPEND_EVENT: Serial.printf("USB SUSPENDED: remote_wakeup_en: %u\n", data->suspend.remote_wakeup_en); break;
      case ARDUINO_USB_RESUME_EVENT:  Serial.println("USB RESUMED"); break;

      default: break;
    }
  }
}
 

/*
static int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
  _flash.partitionRead(Partition, offset + (lba * BLOCK_SIZE), (uint32_t*)buffer, bufsize);
  return bufsize;
}

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
   _flash.partitionEraseRange(Partition, offset + (lba * BLOCK_SIZE), bufsize);

  // Write data to flash memory in blocks from buffer
  _flash.partitionWrite(Partition, offset + (lba * BLOCK_SIZE), (uint32_t*)buffer, bufsize);
 return bufsize;   
}*/

static int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) 
{
    if (Partition == NULL) return 0;
    
    size_t addr = fat_partition->address + (lba * 512) + offset;
    esp_partition_read(fat_partition, addr - fat_partition->address, (uint32_t*)buffer, bufsize);
    return bufsize;
}

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) 
{
    if (Partition == NULL) return 0;
    
    size_t addr = fat_partition->address + (lba * 512) + offset;
    // Borrar sector si es necesario (FFat ya maneja esto, pero para raw write lo hacemos)
    esp_partition_erase_range(fat_partition, (addr - fat_partition->address) & ~(4095), 4096);
    esp_partition_write(fat_partition, addr - fat_partition->address, (uint32_t*)buffer, bufsize);
    return bufsize;
}


static bool onStartStop(uint8_t power_condition, bool start, bool load_eject) {
  return true;
}
