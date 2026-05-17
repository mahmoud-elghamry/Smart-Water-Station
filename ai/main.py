"""
AquaPuer v3.1 — Entry Point

Loads config, sets up logging, starts the AI controller.
If API is enabled, also starts FastAPI on a separate thread.
"""

import threading
import logging
import uvicorn

from config import load_config, setup_logging
from controller import AIController
from api import create_app


def main():
    config = load_config()
    setup_logging(config)
    logger = logging.getLogger("water_station")

    ctrl = AIController(config)

    api_cfg = config.get("api", {})
    if api_cfg.get("enabled", False):
        # Run controller on a background thread, API on main thread
        threading.Thread(target=ctrl.run, name="Controller", daemon=True).start()

        app = create_app(ctrl)
        host = api_cfg.get("host", "0.0.0.0")
        port = api_cfg.get("port", 5000)

        display_host = "localhost" if host in ("0.0.0.0", "::") else host
        logger.info(f"API:  http://{display_host}:{port}/docs")
        logger.info(f"WS:   ws://{display_host}:{port}/ws/live")

        mqtt_cfg = config.get("mqtt", {})
        if mqtt_cfg.get("enabled", False):
            logger.info(f"MQTT: {mqtt_cfg.get('broker', '?')}:{mqtt_cfg.get('port', 1883)}")
            logger.info(f"MQTT Topics: {mqtt_cfg.get('base_topic', 'aquapuer')}/#")

        try:
            uvicorn.run(app, host=host, port=port, log_level="info")
        except KeyboardInterrupt:
            pass
        finally:
            ctrl.shutdown()
    else:
        ctrl.run()


if __name__ == "__main__":
    main()
