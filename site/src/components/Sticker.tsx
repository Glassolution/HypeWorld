// Etiqueta vermelha com duas fitas em X nos cantos
function Tape({ className }: { className: string }) {
  return (
    <svg className={className} viewBox="0 0 60 60" aria-hidden="true">
      <rect x="22" y="-4" width="16" height="68" rx="4" transform="rotate(45 30 30)" />
      <rect x="22" y="-4" width="16" height="68" rx="4" transform="rotate(-45 30 30)" />
    </svg>
  )
}

export default function Sticker({ children }: { children: string }) {
  return (
    <h2 className="sticker">
      <Tape className="tape tape-tl" />
      <span>{children}</span>
      <Tape className="tape tape-br" />
    </h2>
  )
}
