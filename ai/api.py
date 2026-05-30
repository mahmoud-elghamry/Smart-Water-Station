"""FastAPI application factory."""

import asyncio
import logging
from contextlib import asynccontextmanager
from typing import TYPE_CHECKING

from fastapi import FastAPI, WebSocket, WebSocketDisconnect, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field
from typing import Optional

if TYPE_CHECKING:
    from controller import AIController

logger = logging.getLogger("water_station.api")


class CommandRequest(BaseModel):
    cmd: str = Field(..., description="EMERGENCY_STOP, SET_PUMP, RESET …")
    state: Optional[str] = None
    value: Optional[float] = None

class CommandResponse(BaseModel):
    success: bool
    message: str = "Command sent"


class DatasetLabelRequest(BaseModel):
    label: str = Field(..., description="Dataset label, e.g. normal or pump_overload")


def create_app(ctrl: "AIController") -> FastAPI:
    @asynccontextmanager
    async def lifespan(app: FastAPI):
        ctrl._loop = asyncio.get_running_loop()
        logger.info("FastAPI starting")
        yield
        logger.info("FastAPI stopping")

    app = FastAPI(title="AquaPuer API", version="3.1.0", lifespan=lifespan)
    app.add_middleware(CORSMiddleware, allow_origins=["*"], allow_credentials=True,
                       allow_methods=["*"], allow_headers=["*"])

    @app.get("/api/status", tags=["Monitoring"])
    async def status():
        return ctrl.get_status()

    @app.post("/api/command", response_model=CommandResponse, tags=["Control"])
    async def command(req: CommandRequest):
        p = {"cmd": req.cmd}
        if req.state: p["state"] = req.state
        if req.value is not None: p["value"] = req.value
        ctrl.handle_command(p, source="api")
        return CommandResponse(success=True, message=f"'{req.cmd}' sent to all channels")

    @app.get("/api/telemetry", tags=["Monitoring"])
    async def telemetry():
        return ctrl.latest_telemetry

    @app.get("/api/dataset/status", tags=["Dataset"])
    async def dataset_status():
        return ctrl.get_dataset_status()

    @app.post("/api/dataset/label", tags=["Dataset"])
    async def dataset_label(req: DatasetLabelRequest):
        label = ctrl.set_dataset_label(req.label)
        return {"ok": True, "label": label}

    @app.get("/api/alerts", tags=["Security"])
    async def alerts():
        return {
            "alerts": ctrl.monitor.get_active_alerts(),
            "recommendations": ctrl.monitor.get_recommendations(),
            "threat_active": ctrl.last_threat_status,
        }

    @app.get("/api/security/stats", tags=["Security"])
    async def sec_stats():
        return ctrl.monitor.get_stats()

    @app.get("/api/health", tags=["System"])
    async def health():
        return {
            "status": "healthy",
            "serial": ctrl.serial.connected,
            "mqtt": ctrl.mqtt.connected if ctrl.mqtt else False,
            "camera": ctrl.monitor.cap is not None,
            "model": ctrl.monitor.model is not None,
        }

    @app.websocket("/ws/live")
    async def ws_live(ws: WebSocket):
        await ws.accept()
        ctrl.ws_clients.append(ws)
        logger.info(f"WS +1 ({len(ctrl.ws_clients)} total)")
        try:
            await ws.send_json({"type": "initial_status", "data": ctrl.get_status()})
            while ctrl.running:
                await asyncio.sleep(1)
                await ws.send_json({"type": "periodic_update", "data": {
                    "telemetry": ctrl.latest_telemetry,
                    "state": ctrl.latest_state,
                    "alerts": ctrl.monitor.get_active_alerts(),
                    "recommendations": ctrl.monitor.get_recommendations(),
                    "threat": ctrl.last_threat_status,
                }})
        except WebSocketDisconnect:
            pass
        finally:
            if ws in ctrl.ws_clients: ctrl.ws_clients.remove(ws)
            logger.info(f"WS -1 ({len(ctrl.ws_clients)} total)")

    return app
