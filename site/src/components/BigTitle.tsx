import type { CSSProperties } from 'react'
import spiral from '../assets/spiral.png'

// Abertura das páginas internas: frase pequena + palavra gigante esticada
export default function BigTitle({ kicker, big }: { kicker: string; big: string }) {
  return (
    <section className="bigtitle">
      <div className="bigtitle-art" style={{ '--art': `url(${spiral})` } as CSSProperties} aria-hidden="true" />
      <div className="bigtitle-text">
        <span className="bigtitle-kicker">{kicker}</span>
        <span className="bigtitle-big">{big}</span>
      </div>
    </section>
  )
}
