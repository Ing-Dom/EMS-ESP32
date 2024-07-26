#ifndef WebStatusService_h
#define WebStatusService_h

#define HARDWARE_STATUS_SERVICE_PATH "/rest/hardwareStatus"
#define SYSTEM_STATUS_SERVICE_PATH "/rest/systemStatus"

namespace emsesp {

class WebStatusService {
  public:
    WebStatusService(AsyncWebServer * server, SecurityManager * securityManager);

  private:
    void systemStatus(AsyncWebServerRequest * request);
    void hardwareStatus(AsyncWebServerRequest * request);
};

} // namespace emsesp

#endif
