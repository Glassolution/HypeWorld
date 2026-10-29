import { createContext, useContext, useEffect, useState, type ReactNode } from 'react'

export const LANGS = ['pt', 'en', 'es'] as const
export type Lang = (typeof LANGS)[number]

const dict = {
  pt: {
    navAbout: 'Sobre nós',
    navWork: 'Trabalhe conosco',
    download: 'Baixar o jogo',
    heroScroll: 'Role para baixo',
    aboutKicker: 'Quem',
    aboutBig: 'Somos',
    values: ['Os jogadores em primeiro lugar', 'Elevar o padrão', 'Pensar grande, construir sólido', 'Juntos'],
    aboutSticker: 'Quem somos',
    aboutSub: 'Criando o mundo ninja que a gente queria jogar',
    aboutP1: 'HypeWorld é um jogo ninja multiplayer feito em Unreal Engine 5 por um time independente.',
    aboutP2: 'Cada sistema é construído em fases e só segue em frente quando já está jogável: primeiro o movimento, depois o combate, depois os jutsus.',
    aboutVision: 'Nossa visão vai além de um jogo de luta:',
    aboutList: [
      'Movimento ninja que responde na hora: corrida, dash, pulo duplo e corrida na parede.',
      'Combate com combos, desvio, vida e chakra.',
      'Partidas com os amigos sem servidor pago: um hospeda e os outros entram.',
    ],
    workKicker: 'Trabalhe',
    workBig: 'Conosco',
    workSticker: 'Trabalhe conosco',
    workSub: 'Ajude a construir o HypeWorld',
    workP1: 'Procuramos pessoas que gostem de jogos de ação e queiram criar junto: animação, modelagem 3D, programação em Unreal e design de níveis.',
    workP2: 'Mande seu portfólio pelo nosso Discord.',
    legal: 'Aviso legal',
    contact: 'Contato',
    terms: 'Termos de uso',
    privacy: 'Privacidade',
    rights: 'Todos os direitos reservados.',
    social: 'Social',
  },
  en: {
    navAbout: 'About us',
    navWork: 'Work with us',
    download: 'Download the game',
    heroScroll: 'Scroll down',
    aboutKicker: 'Who',
    aboutBig: 'We are',
    values: ['Players first', 'Raise the bar', 'Think big, build solid', 'Together'],
    aboutSticker: 'Who we are',
    aboutSub: 'Building the ninja world we wanted to play',
    aboutP1: 'HypeWorld is a multiplayer ninja game made in Unreal Engine 5 by an independent team.',
    aboutP2: 'Every system is built in phases and only moves on once it is playable: movement first, then combat, then jutsus.',
    aboutVision: 'Our vision goes beyond a fighting game:',
    aboutList: [
      'Ninja movement that responds instantly: sprint, dash, double jump and wall run.',
      'Combat with combos, dodges, health and chakra.',
      'Matches with friends and no paid server: one hosts, the others join.',
    ],
    workKicker: 'Work',
    workBig: 'With us',
    workSticker: 'Work with us',
    workSub: 'Help build HypeWorld',
    workP1: 'We are looking for people who love action games and want to build with us: animation, 3D modeling, Unreal programming and level design.',
    workP2: 'Send your portfolio on our Discord.',
    legal: 'Legal notice',
    contact: 'Contact',
    terms: 'Terms of use',
    privacy: 'Privacy',
    rights: 'All rights reserved.',
    social: 'Social',
  },
  es: {
    navAbout: 'Sobre nosotros',
    navWork: 'Trabaja con nosotros',
    download: 'Descargar el juego',
    heroScroll: 'Desliza hacia abajo',
    aboutKicker: 'Quiénes',
    aboutBig: 'Somos',
    values: ['Los jugadores primero', 'Elevar el nivel', 'Pensar en grande, construir sólido', 'Juntos'],
    aboutSticker: 'Quiénes somos',
    aboutSub: 'Creando el mundo ninja que queríamos jugar',
    aboutP1: 'HypeWorld es un juego ninja multijugador hecho en Unreal Engine 5 por un equipo independiente.',
    aboutP2: 'Cada sistema se construye por fases y solo avanza cuando ya es jugable: primero el movimiento, después el combate, después los jutsus.',
    aboutVision: 'Nuestra visión va más allá de un juego de peleas:',
    aboutList: [
      'Movimiento ninja que responde al instante: carrera, dash, doble salto y carrera por la pared.',
      'Combate con combos, esquiva, vida y chakra.',
      'Partidas con amigos sin servidor de pago: uno hospeda y los demás entran.',
    ],
    workKicker: 'Trabaja',
    workBig: 'Con nosotros',
    workSticker: 'Trabaja con nosotros',
    workSub: 'Ayuda a construir HypeWorld',
    workP1: 'Buscamos personas a las que les gusten los juegos de acción y quieran crear con nosotros: animación, modelado 3D, programación en Unreal y diseño de niveles.',
    workP2: 'Envía tu portafolio por nuestro Discord.',
    legal: 'Aviso legal',
    contact: 'Contacto',
    terms: 'Términos de uso',
    privacy: 'Privacidad',
    rights: 'Todos los derechos reservados.',
    social: 'Social',
  },
}

export type Dict = (typeof dict)['pt']

const Ctx = createContext<{ lang: Lang; setLang: (l: Lang) => void; t: Dict }>({
  lang: 'pt',
  setLang: () => {},
  t: dict.pt,
})

export function LangProvider({ children }: { children: ReactNode }) {
  const [lang, setLang] = useState<Lang>(() => {
    try {
      const saved = localStorage.getItem('lang') as Lang | null
      if (saved && LANGS.includes(saved)) return saved
    } catch {
      /* sem storage */
    }
    return 'pt'
  })

  useEffect(() => {
    document.documentElement.lang = lang === 'pt' ? 'pt-BR' : lang
    try {
      localStorage.setItem('lang', lang)
    } catch {
      /* sem storage */
    }
  }, [lang])

  return <Ctx.Provider value={{ lang, setLang, t: dict[lang] }}>{children}</Ctx.Provider>
}

// eslint-disable-next-line react-refresh/only-export-components
export const useI18n = () => useContext(Ctx)
