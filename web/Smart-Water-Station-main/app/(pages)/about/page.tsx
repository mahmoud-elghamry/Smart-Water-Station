import Image from 'next/image'
import { StaticImageData } from 'next/image'
import card from "../../../Project photos/card 1.jpg"
import sea from "../../../Project photos/card 2.jpg"
import station from "../../../Project photos/card 3.jpg"
import bridge from "../../../Project photos/card 4.jpg"
import team from "../../../Project photos/team.jpg"
import { ReactNode } from 'react'

const cards: { title: string; emoji: string; image: StaticImageData; imageClass?: string; custom: ReactNode }[] = [
  {
    title: "About the System",
    emoji: "💧",
    image: card,
    custom: (
      <div className="text-sm leading-relaxed space-y-2 text-white/90">
        <p className="font-medium text-white italic">
          "Smart Water Reuse & UF Filtration System"
        </p>
        <p>A smart platform designed to optimize water reuse in filtration units by:</p>
        <ul className="space-y-1 mt-2">
          <li className="flex items-center gap-2">📡 Collecting real-time sensor data</li>
          <li className="flex items-center gap-2">🤖 Analyzing it with Artificial Intelligence</li>
          <li className="flex items-center gap-2">♻️ Reducing water waste</li>
          <li className="flex items-center gap-2">⚡ Improving operational efficiency</li>
        </ul>
      </div>
    ),
  },
  {
    title: "System Goals",
    emoji: "🎯",
    image: sea,
    custom: (
      <ul className="text-sm text-white/90 space-y-2">
        <li className="flex items-start gap-2">
          <span>💦</span>
          <span>Reduce water waste across all filtration stages</span>
        </li>
        <li className="flex items-start gap-2">
          <span>📈</span>
          <span>Improve operational efficiency with data-driven decisions</span>
        </li>
        <li className="flex items-start gap-2">
          <span>🔮</span>
          <span>Provide predictive insights for proactive maintenance</span>
        </li>
        <li className="flex items-start gap-2">
          <span>🌱</span>
          <span>Promote sustainability in water management systems</span>
        </li>
      </ul>
    ),
  },
  {
    title: "Key Features",
    emoji: "⚙️",
    image: station,
    custom: (
      <div className="text-sm text-white/90 space-y-3">
        <div>
          <p className="font-semibold text-white mb-1">📊 Sensor Monitoring</p>
          <ul className="space-y-1 pl-2">
            <li className="flex items-center gap-2">〰️ TDS & Turbidity</li>
            <li className="flex items-center gap-2">💨 Flow & Pressure</li>
          </ul>
        </div>
        <div>
          <p className="font-semibold text-white mb-1">🧠 Intelligence</p>
          <ul className="space-y-1 pl-2">
            <li className="flex items-center gap-2">🤖 AI filter maintenance predictions</li>
            <li className="flex items-center gap-2">🚨 Smart critical condition alerts</li>
          </ul>
        </div>
        <div>
          <p className="font-semibold text-white mb-1">🎛️ Control</p>
          <ul className="space-y-1 pl-2">
            <li className="flex items-center gap-2">🔌 Pump On/Off + AI mode</li>
            <li className="flex items-center gap-2">🖥️ Clean, user-friendly interface</li>
          </ul>
        </div>
      </div>
    ),
  },
  {
    title: "Project Vision",
    emoji: "🌊",
    image: bridge,
    custom: (
      <div className="text-sm text-white/90 space-y-3">
        <p className="italic text-white font-medium">
          "Intelligent water filtration for a sustainable future."
        </p>
        <ul className="space-y-2">
          <li className="flex items-start gap-2">
            <span>🌍</span>
            <span>Conserve water resources at scale</span>
          </li>
          <li className="flex items-start gap-2">
            <span>💡</span>
            <span>Empower operators with actionable insights</span>
          </li>
          <li className="flex items-start gap-2">
            <span>🏆</span>
            <span>Ensure optimal performance and reliability</span>
          </li>
          <li className="flex items-start gap-2">
            <span>♻️</span>
            <span>Drive sustainability in industrial water use</span>
          </li>
        </ul>
      </div>
    ),
  },
  {
    title: "Team Members",
    emoji: "👥",
    image: team,
    imageClass: "w-full h-64 object-contain object-center bg-[#3a5d96]",
    custom: (
      <div className="text-sm text-white/90 space-y-2">
        <div className="flex items-start gap-2">
          <span>💻</span>
          <div><span className="font-semibold text-white">Nourhan Fathallah</span> — Web Developer (Front-end)</div>
        </div>
        <div className="flex items-start gap-2">
          <span>🎨</span>
          <div><span className="font-semibold text-white">Doha & Omar</span> — UI/UX Designers</div>
        </div>
        <div className="flex items-start gap-2">
          <span>🔧</span>
          <div><span className="font-semibold text-white">Mahmoud & Eslam</span> — Embedded Systems</div>
        </div>
        <div className="flex items-start gap-2">
          <span>🤖</span>
          <div><span className="font-semibold text-white">Basam</span> — AI & Data Analyst</div>
        </div>
        <div className="flex items-start gap-2">
          <span>🌐</span>
          <div><span className="font-semibold text-white">Ahmed Elmenofy & Fathy</span> — Network</div>
        </div>
        <div className="flex items-start gap-2">
          <span>⚗️</span>
          <div><span className="font-semibold text-white">Abrar & Nourhan</span> — Chemical Work & Treatments</div>
        </div>
      </div>
    ),
  },
];

export default function About() {
  return (
    <section className="px-6 py-16 max-w-7xl mx-auto">
      {/* Header */}
      <div className="mb-12 text-center">
        <h2 className="text-4xl font-bold text-heading mb-3 text-blue-950">About AquaPuer</h2>
        <p className="text-body max-w-2xl mx-auto text-base text-blue-950">
          A next-generation platform built to make water filtration smarter, leaner, and more sustainable.
        </p>
      </div>

      {/* Card Grid */}
      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-6">
        {cards.map((card_item, index) => (
          <div
            key={index}
            style={{ backgroundColor: '#456DAE' }}
            className="group border border-white/20 rounded-base shadow-xs overflow-hidden transition-all duration-300 hover:shadow-xl hover:shadow-[#456DAE]/40 hover:-translate-y-1 hover:border-white/40 cursor-pointer"
          >
            {/* Image */}
            <div className="overflow-hidden">
              <Image
                src={card_item.image}
                alt={card_item.title}
                width={400}
                height={220}
                className={`transition-transform duration-500 group-hover:scale-105 ${
                  card_item.imageClass ?? "w-full h-48 object-cover"
                }`}
              />
            </div>

            {/* Content */}
            <div className="p-6">
              <h5 className="mb-3 text-xl font-semibold tracking-tight text-white transition-colors duration-200 group-hover:text-white/80 flex items-center gap-2">
                <span>{card_item.emoji}</span>
                {card_item.title}
              </h5>
              {card_item.custom}
            </div>

            {/* Bottom accent bar */}
            <div className="h-1 w-0 bg-white/60 transition-all duration-300 group-hover:w-full" />
          </div>
        ))}
      </div>
    </section>
  )
}