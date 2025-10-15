
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
  if (!mscStorage || lba >= MSC_SECTOR_COUNT) {
    return 0;
  }
  
  uint32_t storage_offset = lba * MSC_SECTOR_SIZE + offset;
  uint32_t bytes_to_copy = min(bufsize, (MSC_SECTOR_COUNT * MSC_SECTOR_SIZE) - storage_offset);
  
  if (bytes_to_copy > 0) {
    memcpy(buffer, mscStorage + storage_offset, bytes_to_copy);
  }
  
  return bytes_to_copy;
}

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
  if (!mscStorage || lba >= MSC_SECTOR_COUNT) {
    return 0;
  }
  
  uint32_t storage_offset = lba * MSC_SECTOR_SIZE + offset;
  uint32_t bytes_to_copy = min(bufsize, (MSC_SECTOR_COUNT * MSC_SECTOR_SIZE) - storage_offset);
  
  if (bytes_to_copy > 0) {
    memcpy(mscStorage + storage_offset, buffer, bytes_to_copy);
  }
  
  return bytes_to_copy;
}

static bool onStartStop(uint8_t power_condition, bool start, bool load_eject) {
  USBSerial.printf("MSC Start/Stop: power_condition=%d, start=%d, load_eject=%d\n", 
                power_condition, start, load_eject);
  mscReady = start;
  return true;
}
