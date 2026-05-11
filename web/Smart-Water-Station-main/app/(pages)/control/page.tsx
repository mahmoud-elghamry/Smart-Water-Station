"use client"
import React from 'react'
import PageHeader from "@/_components/fixed-components/PageHeader/PageHeader"
import {
  Field,
  FieldContent,
  FieldLabel,
} from "../../../_components/ui/field"
import { Switch } from "../../../_components/ui/switch"
import { Power, ClockAlert, ListFilter } from 'lucide-react'
import { ToggleGroup, ToggleGroupItem } from "../../../_components/ui/toggle-group"
import { Card, CardDescription, CardHeader, CardTitle } from '@/_components/ui/card'
import { Badge } from '@/_components/ui/badge'
import { Separator } from '@/_components/ui/separator'
import { useWaterStation } from '@/lib/use-water-station'

export default function Control() {
  const { state, transport, sendControl } = useWaterStation()

  return (
    <>
      <PageHeader />
      <div className="px-4 sm:px-10 lg:px-20">

      <div className="mt-8">
        <h1 className="text-2xl sm:text-3xl font-bold pb-2 text-[#456DAE]">
          Main Pump Control
        </h1>
        <p className="text-[#456DAE] text-sm sm:text-base">
          Live control connected to ESP32
        </p>
      </div>

      <div className="mt-4 flex flex-col gap-4">
        <div className={`${state.pressure > 3 ? "bg-[#F9E8E8] border-red-200 text-red-600" : "bg-green-50 border-green-200 text-green-700"} border rounded-md px-4 py-3 text-sm font-medium`}>
          {state.pressure > 3 ? "Warning: Water Pressure is above safe level!" : "Pressure is within the safe test range."}
        </div>

        <div className="bg-white border border-blue-300 rounded-md px-4 py-4 text-[#456DAE] text-sm">
          <Field orientation="horizontal" className="w-full">
            <FieldContent>
              <FieldLabel htmlFor="switch-pump" className="flex items-center gap-2 text-[#456DAE] font-semibold">
                <Power className="h-4 w-4" />
                {state.pumpOn ? "Stop Pump" : "Start Pump"}
              </FieldLabel>
            </FieldContent>
            <Switch
              id="switch-pump"
              checked={state.pumpOn}
              onCheckedChange={(checked) => sendControl({ pumpOn: checked })}
              className="data-[state=unchecked]:bg-gray-200 data-[state=checked]:bg-[#456DAE] rounded-full transition-colors duration-200 [&>span]:rounded-full [&>span]:shadow-md [&>span]:transition-transform [&>span]:duration-200"
            />
          </Field>
        </div>
      </div>
      <Separator className="my-6 bg-blue-950"/>

      <div className="mt-8">
        <h3 className="text-2xl sm:text-3xl font-bold pb-2 text-[#456DAE]">
          Operation Mode
        </h3>

        <ToggleGroup
          type="single"
          value={state.mode}
          onValueChange={(val) => val && sendControl({ mode: val as "manual" | "ai" })}
          className="bg-[#DDE3EF] rounded-xl p-1 w-full mt-4 flex"
        >
          <ToggleGroupItem
            value="manual"
            className="flex-1 rounded-lg text-[#456DAE] font-semibold data-[state=on]:bg-white data-[state=on]:shadow-sm"
          >
            Manual
          </ToggleGroupItem>
          <ToggleGroupItem
            value="ai"
            className="flex-1 rounded-lg text-[#456DAE] font-semibold data-[state=on]:bg-white data-[state=on]:shadow-sm"
          >
            AI Mode
          </ToggleGroupItem>
        </ToggleGroup>
      </div>

      <div className="grid grid-cols-1 sm:grid-cols-2 gap-6 mt-6 pb-25">
        <Card className="w-full px-4 sm:px-8 py-6 bg-[#F9F9F9] pb-15">
          <CardHeader>
            <CardTitle className="flex items-center gap-2 text-[#456DAE]">
              <ClockAlert className="h-4 w-4" /> Water Pressure
            </CardTitle>
            <CardDescription className="text-[#456DAE] mt-2">
              Current pressure from ESP32 telemetry
              <span className="mt-3 block text-3xl font-bold text-[#456DAE]">
                {state.pressure.toFixed(1)} bar
              </span>
            </CardDescription>
          </CardHeader>
        </Card>

        <Card className="w-full px-4 sm:px-8 py-6 bg-[#F9F9F9]">
          <CardHeader>
            <CardTitle className="flex items-center gap-2 text-[#456DAE]">
              <ListFilter className="h-4 w-4" /> Filter Status
            </CardTitle>
            <CardDescription className="mt-2">
              <Badge variant="secondary" className="bg-green-100 text-green-700 border border-green-300">
                Active
              </Badge>
              <span className="ml-2 text-[#456DAE]">{transport === "websocket" ? "Connected" : "Simulation"}</span>
            </CardDescription>
          </CardHeader>
        </Card>

      </div>

      </div>
    </>
  )
}
