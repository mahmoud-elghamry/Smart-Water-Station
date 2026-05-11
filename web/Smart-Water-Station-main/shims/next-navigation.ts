import { useEffect, useState } from "react";

function currentPathname() {
  return window.location.pathname || "/";
}

export function usePathname() {
  const [pathname, setPathname] = useState(currentPathname);

  useEffect(() => {
    const update = () => setPathname(currentPathname());
    window.addEventListener("popstate", update);
    return () => window.removeEventListener("popstate", update);
  }, []);

  return pathname;
}
