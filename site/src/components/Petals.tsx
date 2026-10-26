import { useEffect, useRef } from 'react'

type Petal = { x: number; y: number; s: number; vx: number; vy: number; r: number; vr: number; a: number }

// Pétalas caindo: fundo provisório da home até existir o vídeo do jogo
export default function Petals() {
  const ref = useRef<HTMLCanvasElement>(null)

  useEffect(() => {
    const cv = ref.current
    const ctx = cv?.getContext('2d')
    if (!cv || !ctx) return

    const color = getComputedStyle(document.documentElement).getPropertyValue('--accent').trim() || '#e0476b'
    const reduce = matchMedia('(prefers-reduced-motion: reduce)').matches
    let w = 0
    let h = 0
    let petals: Petal[] = []
    let raf = 0

    const make = (anywhere: boolean): Petal => ({
      x: Math.random() * w * 1.2 - w * 0.1,
      y: anywhere ? Math.random() * h : -20,
      s: 5 + Math.random() * 9,
      vx: 0.3 + Math.random() * 0.9,
      vy: 0.4 + Math.random() * 0.9,
      r: Math.random() * Math.PI * 2,
      vr: (Math.random() - 0.5) * 0.04,
      a: 0.35 + Math.random() * 0.55,
    })

    const size = () => {
      const r = cv.getBoundingClientRect()
      const d = Math.min(devicePixelRatio || 1, 2)
      w = r.width
      h = r.height
      cv.width = w * d
      cv.height = h * d
      ctx.setTransform(d, 0, 0, d, 0, 0)
      petals = Array.from({ length: Math.round(w / 40) }, () => make(true))
    }

    const draw = () => {
      ctx.clearRect(0, 0, w, h)
      ctx.fillStyle = color
      for (const p of petals) {
        ctx.save()
        ctx.globalAlpha = p.a
        ctx.translate(p.x, p.y)
        ctx.rotate(p.r)
        ctx.beginPath()
        ctx.ellipse(0, 0, p.s, p.s * 0.55, 0, 0, Math.PI * 2)
        ctx.fill()
        ctx.restore()
        if (!reduce) {
          p.x += p.vx + Math.sin(p.y / 60) * 0.4
          p.y += p.vy
          p.r += p.vr
          if (p.y > h + 20 || p.x > w + 20) Object.assign(p, make(false))
        }
      }
      if (!reduce) raf = requestAnimationFrame(draw)
    }

    const onResize = () => {
      size()
      if (reduce) draw()
    }

    size()
    draw()
    addEventListener('resize', onResize)
    return () => {
      cancelAnimationFrame(raf)
      removeEventListener('resize', onResize)
    }
  }, [])

  return <canvas ref={ref} className="petals" aria-hidden="true" />
}
