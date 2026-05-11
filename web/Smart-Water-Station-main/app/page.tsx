"use client";

import Image from "next/image";
import Link from "next/link";
import heroImage from "../Project photos/card 3.jpg";
import featureOne from "../Project photos/card 1.jpg";
import featureTwo from "../Project photos/card 2.jpg";
import featureThree from "../Project photos/card 4.jpg";
import featureFour from "../Project photos/card 5.jpg";


type FeatureItem = {
  title: string;
  description: string;
  primaryImage: string;
  secondaryImage: string;
  reverse?: boolean;
};

const features: FeatureItem[] = [
  {
    title: "Smart Water Reuse System Powered by AI",
    description: "Automatically filter and redistribute greywater for non-potable use.",
    primaryImage: featureOne,
    secondaryImage: featureTwo,
  },
  {
    title: "Smart Water Reuse System Powered by AI",
    description: "Real-time sensing and adaptive control for cleaner water systems.",
    primaryImage: featureThree,
    secondaryImage: featureFour,
    reverse: true,
  },
  {
    title: "Smart Water Reuse System Powered by AI",
    description: "Predictive insights and alerts before issues impact your network.",
    primaryImage: featureTwo,
    secondaryImage: featureOne,
  },
];

export default function HomePage() {
  return (
    <main className="w-full text-white">

      <section className="relative h-[62vh] min-h-[420px] w-full overflow-hidden">
        <Image
          src={heroImage}
          alt="Water technology network"
          fill
          priority
          className="object-cover"
          sizes="100vw"
        />
        <div className="absolute inset-0 bg-[#0f2c5f]/55" />

        <div className="relative z-10 mx-auto flex h-full max-w-5xl flex-col items-center justify-center px-4 text-center">
          <h1 className="mb-4 text-5xl font-bold md:text-6xl">Welcome to AquaPuer</h1>
          <p className="mb-8 max-w-2xl text-xl text-blue-100 md:text-3xl">
            Clean Water, Smart Monitoring.
            <br />
            Join us in making water smarter and safer for everyone.
          </p>
          <Link
            href="/login"
            className="rounded-md bg-[#6b8bc7] px-10 py-3 text-2xl font-semibold text-white shadow-lg transition hover:bg-[#5c7cb8]"
          >
            Get Started
          </Link>
        </div>
      </section>

      {features.map((feature, index) => (
        <section
          key={index}
          className={`relative overflow-hidden px-6 py-16 md:py-20 ${
            index % 2 === 0
              ? "bg-gradient-to-br from-[#5f86cc] via-[#7399da] to-[#89a8de]"
              : "bg-gradient-to-br from-[#5b83cd] via-[#6e93d2] to-[#84a5d9]"
          }`}
        >
          <div className="pointer-events-none absolute inset-0 bg-[radial-gradient(circle_at_20%_20%,rgba(255,255,255,0.15),transparent_45%)]" />
          <div className="mx-auto grid min-h-[560px] max-w-7xl items-center gap-10 lg:grid-cols-2 lg:gap-16">
            <div className={`space-y-6 ${feature.reverse ? "lg:order-2 lg:pl-10" : "lg:order-1"}`}>
              <h2 className="max-w-xl text-5xl font-bold leading-tight md:text-7xl">{feature.title}</h2>
              <p className="max-w-lg text-xl text-blue-100 md:text-2xl">{feature.description}</p>
              {/* show button only on last section */}
        {index === features.length - 1 && (
          <Link
            href="/about"
className="rounded-md bg-[#6b8bc7] px-10 py-3 text-2xl font-semibold text-white shadow-lg transition hover:bg-[#5c7cb8]"
          >
            Learn More About Us →
          </Link>
        )}
            </div>
              

            <div className={`relative h-[320px] w-full sm:h-[420px] ${feature.reverse ? "lg:order-1" : "lg:order-2"}`}>
              <div className="absolute inset-x-4 top-2 h-3/4 overflow-hidden rounded-2xl shadow-2xl sm:inset-x-8">
                <Image
                  src={feature.primaryImage}
                  alt={feature.title}
                  fill
                  className="object-cover"
                  sizes="(max-width: 1024px) 90vw, 40vw"
                />
              </div>

              <div
                className={`absolute bottom-0 h-2/3 w-3/4 overflow-hidden rounded-2xl shadow-2xl ${
                  feature.reverse ? "left-0" : "right-0"
                }`}
              >
                <Image
                  src={feature.secondaryImage}
                  alt={`${feature.title} detail`}
                  fill
                  className="object-cover"
                  sizes="(max-width: 1024px) 70vw, 30vw"
                />
              </div>
            </div>
          </div>
        </section>
      ))}
    </main>
  );
}
