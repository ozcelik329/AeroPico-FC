import { motion } from "motion/react";
import { springSnappy } from "../../utils/motionPresets.js";

function MacIcon() {
  return (
    <svg aria-hidden="true" viewBox="0 0 24 24" className="platform-icon">
      <path
        fill="currentColor"
        d="M15.4 3.1c.1 1.1-.3 2.1-1 2.9-.8.9-1.8 1.4-2.8 1.3-.1-1 .3-2 1-2.8.8-.9 1.8-1.4 2.8-1.4ZM20.1 17.2c-.4 1-1 2-1.7 2.9-.9 1.2-1.8 1.9-2.7 1.9-.4 0-.9-.1-1.5-.4-.6-.2-1.1-.4-1.6-.4s-1 .1-1.6.4c-.6.3-1.1.4-1.5.4-1 0-1.9-.7-2.8-2-1.4-2-2.1-4.2-2.1-6.5 0-1.5.4-2.8 1.2-3.8.8-1 1.9-1.6 3.1-1.6.5 0 1.1.1 1.8.4.7.3 1.1.4 1.4.4.2 0 .7-.2 1.5-.5.8-.3 1.5-.4 2.1-.4 1.7.1 3 1 3.9 2.6-1.5.9-2.2 2.2-2.2 3.8 0 .8.2 1.5.7 2.1.4.3.7.5 1 .7Z"
      />
    </svg>
  );
}

function WindowsIcon() {
  return (
    <svg aria-hidden="true" viewBox="0 0 24 24" className="platform-icon">
      <path fill="currentColor" d="M3 5.2 10.8 4v7.2H3V5.2Zm9.2-1.4L21 2.5v8.7h-8.8V3.8ZM3 12.8h7.8V20L3 18.8v-6Zm9.2 0H21v8.7l-8.8-1.3v-7.4Z" />
    </svg>
  );
}

const icons = {
  macos: MacIcon,
  windows: WindowsIcon,
};

export default function PlatformDownloadButtons({ items, className = "" }) {
  return (
    <div className={`platform-downloads ${className}`}>
      {items.map((item) => {
        const Icon = icons[item.platform] || WindowsIcon;

        return (
          <motion.a
            className="platform-download-button"
            href={item.href || "#"}
            aria-label={item.ariaLabel}
            onClick={item.href ? undefined : (event) => event.preventDefault()}
            key={item.platform}
            whileHover={{ y: -2 }}
            whileTap={{ scale: 0.97, y: 0 }}
            transition={springSnappy}
          >
            <Icon />
            <span>
              <small>{item.kicker}</small>
              <strong>{item.label}</strong>
            </span>
          </motion.a>
        );
      })}
    </div>
  );
}
