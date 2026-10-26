import { useEffect, useRef, useState } from 'react'
import logo from '../assets/logo.png'
import Petals from './Petals'
import { HERO_VIDEO } from '../config'
import { useI18n } from '../i18n'

const clamp = (v: number) => Math.min(1, Math.max(0, v))

export default function Hero() {
  const { t } = useI18n()
  const ref = useRef<HTMLElement>(null)
  const [p, setP] = useState(0)
  const [hasVideo, setHasVideo] = useState(false)

  useEffect(() => {
    const onScroll = () => {
      const el = ref.current
      if (!el) return
      const r = el.getBoundingClientRect()
      setP(clamp(-r.top / (r.height - innerHeight)))
    }
    onScroll()
    addEventListener('scroll', onScroll, { passive: true })
    addEventListener('resize', onScroll)
    return () => {
      removeEventListener('scroll', onScroll)
      removeEventListener('resize', onScroll)
    }
  }, [])

  // A logo surge na primeira metade da rolagem e cresce até o fim, como no Zenkai
  const logoIn = clamp(p / 0.45)
  const logoStyle = {
    opacity: logoIn,
    transform: `translate(-50%, -50%) scale(${0.7 + logoIn * 0.3 + p * 0.25})`,
  }
  const sceneStyle = { transform: `scale(${1.12 - p * 0.12})` }

  return (
    <section className="hero" ref={ref}>
      <div className="hero-sticky">
        <div className="hero-scene" style={sceneStyle}>
          <div className="hero-fallback">
            <Petals />
          </div>
          <video
            className={hasVideo ? 'ready' : ''}
            src={HERO_VIDEO}
            autoPlay
            muted
            loop
            playsInline
            onLoadedData={() => setHasVideo(true)}
            onError={() => setHasVideo(false)}
          />
        </div>
        <div className="hero-veil" />
        <img className="hero-logo" src={logo} alt="HypeWorld" style={logoStyle} />
        <span className="hero-scroll" style={{ opacity: 1 - logoIn }}>
          {t.heroScroll}
        </span>
      </div>
    </section>
  )
}
