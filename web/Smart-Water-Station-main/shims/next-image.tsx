import type { ImgHTMLAttributes } from "react";

export type StaticImageData = string;

type ImageProps = Omit<ImgHTMLAttributes<HTMLImageElement>, "src"> & {
  src: string | StaticImageData;
  fill?: boolean;
  priority?: boolean;
  sizes?: string;
};

export default function Image({
  src,
  fill,
  priority,
  style,
  loading,
  ...props
}: ImageProps) {
  return (
    <img
      src={src}
      loading={priority ? "eager" : loading}
      style={
        fill
          ? {
              position: "absolute",
              inset: 0,
              width: "100%",
              height: "100%",
              ...style,
            }
          : style
      }
      {...props}
    />
  );
}
