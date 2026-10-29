import logo from '../assets/logo.png'

export default function Brand({ size = 'md' }: { size?: 'md' | 'lg' }) {
  return (
    <span className={`brand brand-${size}`}>
      <img src={logo} alt="HypeWorld" />
      <span className="brand-badge">GAME</span>
    </span>
  )
}
